const http = require('http');
const util = require('util');
const exec = util.promisify(require('child_process').exec);
const getPort = require('get-port');
const express = require('express');
const socketio = require('socket.io');
const fs = require('fs');

const DebuggerInterface = require('./dbgif');

const uiRoot = 'debugger-ui/dist';
const objdump = process.env.OBJDUMP_PATH || 'mips64-elf-objdump';
const appBinary = process.env.APP_BINARY_PATH || '../example/ed64logdemo.out';

const app = express();
const server = http.Server(app);
const io = socketio(server);
app.use(express.static(uiRoot));

function parseDisassembly(buffer) {
  const codeStart = buffer.indexOf('Disassembly of section ..code');
  if (codeStart == -1) {
    throw new Error(`couldn't parse disassembly`);
  }
  const addresses = {};
  const functions = {};
  const lines = buffer.slice(codeStart).split('\n');

  let errors = 0;
  for (const line of lines) {
    if (line[0] != '8') {
      // n64 kseg0 addresses start with '8'
      continue;
    }
    const address = line.slice(0, 8);
    const matches = line.slice(9).match(/\<(\w+)(\+\w+)?\>\s(.*)/);
    if (matches) {
      const [input, fnName, offset, disasm] = matches;

      addresses[address] = {
        fnName,
        offset,
        disasm,
      };

      if (fnName) {
        functions[fnName] = functions[fnName] || [];
        functions[fnName].push(address);
      }
    } else {
      if (errors < 10) {
        // console.log('failed to parse', line.slice(9));
      }
      errors++;
    }
  }
  return {addresses, functions};
}

class Server {
  dbgif = null;

  disassemblyPromise = null;

  // debugger state
  state = {
    threads: {},
    atBreakpoint: null,
    serverErrors: [],
  };

  setState(stateUpdate) {
    Object.assign(this.state, stateUpdate);
    fs.writeFile(
      'laststate.json',
      JSON.stringify(this.state),
      {encoding: 'utf8'},
      () => {}
    );
    io.emit('state', this.state);
  }

  handleError(message, error) {
    this.state.serverErrors.push({message, error});
    console.error(message, error);
  }

  attachDebuggerInferfaceHandlers(dbgif) {
    dbgif.on('break', (threadID) => {
      this.setState({
        atBreakpoint: threadID,
      });
    });
    dbgif.on('threadstate', (threadState) => {
      this.setState({
        threads: {
          ...this.state.threads,
          [threadState.id]: threadState,
        },
      });
    });
    dbgif.on('trace', (line) => {
      io.emit('log', `TRACE=[${line.length - 6} bytes]`);
    });
    dbgif.on('log', (line) => {
      io.emit('log', line);
    });
    dbgif.on('error', (err) => {
      this.handleError('debugger interface error', err);
    });
  }

  handleCommand(cmd, data) {
    switch (cmd) {
      case 'b':
      case 's':
      case 'r':
        this.setState({
          atBreakpoint: null,
        });
        break;
    }
    this.dbgif.sendCommand(cmd, data);
  }

  attachClientHandlers(socket) {
    // send current state on connect
    socket.emit('state', this.state);

    // subscribe to handle commands send from client
    socket.on('cmd', ({cmd, data}) => {
      this.handleCommand(cmd, data);
    });
  }

  async startServer(httpPort) {
    const dbgif = new DebuggerInterface();
    this.dbgif = dbgif;

    try {
      await dbgif.start();
      this.attachDebuggerInferfaceHandlers(dbgif);
    } catch (err) {
      console.error(err);
      if (process.env.NODE_ENV === 'development') {
        // proceed without serial connection, for ui development purposes
        console.error(
          'unable to open serial port to ftdi device, is it connected?'
        );

        // load last state
        try {
          Object.assign(
            this.state,
            JSON.parse(fs.readFileSync('laststate.json', {encoding: 'utf8'}))
          );
        } catch (err) {
          console.log(err);
        }
      } else {
        throw new Error(
          'unable to open serial port to ftdi device, is it connected?'
        );
      }
    }

    server.listen(httpPort);

    app.get('/', (req, res) => {
      if (process.env.NODE_ENV === 'development') {
        res.redirect(301, `http://127.0.0.1:3000/?port=${httpPort}`);
      } else {
        res.sendFile(path.join(__dirname, uiRoot, 'index.html'));
      }
    });

    app.get('/disassembly', (req, res) => {
      const promise = this.getDisassembly();

      res.set('Access-Control-Allow-Origin', '*');

      promise
        .then((result) => {
          res.status(200).send(result);
        })
        .catch((err) => {
          res
            .status(503)
            .type('text')
            .send(err.stack);
        });
    });

    io.on('connection', (socket) => {
      this.attachClientHandlers(socket);
    });

    console.log(`server running at http://127.0.0.1:${httpPort}`);
    if (process.env.NODE_ENV === 'development') {
      exec(`open http://127.0.0.1:${httpPort}/`);
    }
  }

  getDisassembly() {
    if (this.disassemblyPromise == null) {
      const promise = exec(
        `${objdump} --disassemble   --prefix-addresses  --wide ${appBinary}`,
        {
          maxBuffer: 1024 * 1024 * 16, // 16mb
        }
      )
        .then(({stdout, stderr}) => {
          return parseDisassembly(stdout);
        })
        .catch((err) => {
          this.handleError('failed to get disassembly using objdump', err);
        });
      this.disassemblyPromise = promise;
    }

    return this.disassemblyPromise;
  }
}

getPort()
  .then((httpPort) => new Server().startServer(httpPort))
  .catch((err) => {
    console.error(err);
    process.exit(1);
  });
