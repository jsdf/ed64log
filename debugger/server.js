const http = require('http');
const DebuggerInterface = require('./dbgif');

const dbgif = new DebuggerInterface();
dbgif.start();

function streamToPromise(stream) {
  return new Promise((resolve, reject) => {
    let data = '';
    stream.on('data', (chunk) => {
      data += chunk.toString();
    });
    stream.on('end', () => {
      resolve(data);
    });
    stream.on('error', (err) => {
      reject(err);
    });
  });
}

class Server {
  dbgif = null;

  // debugger state
  threads = {};
  httpServer = null;
  atBreakpoint = false;

  constructor(dbgif) {
    this.dbgif = dbgif;
    this.attachHandlers(dbgif);
  }

  attachHandlers(dbgif) {
    dbgif.on('break', (threadID) => {
      this.atBreakpoint = true;
    });
    dbgif.on('threadstate', (threadState) => {
      this.threads[threadState.id] = threadState;
    });
  }

  handleCommand(cmd, data) {
    switch (cmd) {
      case 'b':
      case 's':
      case 'r':
        this.atBreakpoint = false;
    }
    this.dbgif.sendCommand(cmd, data);
  }

  startServer(port) {
    if (this.httpServer) {
      throw new Error('server already started');
    }
    this.httpServer = http.createServer(async (req, res) => {
      try {
        let responseData = null;
        const requestBody = await streamToPromise(req);
        let requestBodyData = null;
        if (requestBody.length) {
          requestBodyData = JSON.parse(requestBody);
        }
        switch (req.url) {
          case '/threads':
            responseData = this.threads;
            break;
          case '/command':
            this.handleCommand(requestBodyData.cmd, requestBodyData.data);
            break;
          default:
            console.log(`404 ${req.url}`);
            res.writeHead(404, {'Content-Type': 'text/plain'});
            res.write('404 not found');
            return res.end();
        }
        res.writeHead(200, {'Content-Type': 'application/json'});
        res.write(JSON.stringify(responseData));
        return res.end();
      } catch (err) {
        console.log(`503 ${req.url} ${err}`);
        res.writeHead(503, {'Content-Type': 'text/plain'});
        res.write(err.stack);
        return res.end();
      }
    });
    this.httpServer.listen(port);

    console.log(`server running at http://127.0.0.1:${port}`);
  }
}

const server = new Server(dbgif);
server.startServer(9001);
