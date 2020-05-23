#!/usr/bin/env node
const SerialPort = require('serialport');

const EventEmitter = require('events');

function trimNullBytes(data) {
  const trimmed = [];
  for (const byte of data) {
    if (byte != 0) {
      trimmed.push(byte);
    }
  }
  return Buffer.from(trimmed);
}

class DebuggerInterface extends EventEmitter {
  async start() {
    if (this.port) {
      throw new Error('already started DebuggerInterface');
    }

    const portInfos = await SerialPort.list();
    const ftdiPortInfo = portInfos.find(
      (info) => info.vendorId === '0403' && info.productId === '6001'
    );

    if (!ftdiPortInfo) {
      throw new Error('ftdiPortInfo not found');
    }

    const port = new SerialPort(ftdiPortInfo.path, {
      baudRate: 57600,
    });
    this.port = port;

    this.attachParser();

    port.on('error', function(err) {
      console.error(err);
    });

    process.on('SIGINT', function() {
      console.log('exiting...');

      port.pause();
      port.drain((err) => {
        if (err) console.error(err);

        port.close((err) => {
          if (err) console.error(err);
          process.exit(0);
        });
      });
    });
  }

  attachParser() {
    const port = this.port;
    let buffer = Buffer.alloc(0);

    const visitReceivedLine = (line) => {
      // here we can intercept in-band signalling
      if (line.startsWith('EDBG')) {
        if (line.startsWith('EDBG=break')) {
          const [msgType, id] = line.split(' ');
          this.emit('break', id);
        } else if (line.startsWith('EDBG=thread')) {
          const [
            msgType,
            id,
            stateName,
            priority,
            pc,
            ...stacktrace
          ] = line.split(' ');
          this.emit('threadstate', {
            id,
            stateName,
            priority,
            pc,
            stacktrace,
          });
        }
      } else if (line.startsWith('TRACE=')) {
        console.log(`TRACE=[${line.length - 6} bytes]`);
      } else {
        process.stdout.write(line + '\n');
      }
    };

    function handleData(data) {
      // console.log(data.toString());
      let trimmed = trimNullBytes(data);
      // console.log({
      //   data,
      //   untrimmed: data.toString(),
      //   trimmed: trimmed.toString(),
      // });
      buffer = Buffer.concat([buffer, trimmed]);
      let pos = 0;
      let endPos = 0;
      while ((endPos = buffer.indexOf('\n', pos)) > -1) {
        const line = buffer.slice(pos, endPos).toString();
        visitReceivedLine(line);
        pos = endPos + 1; // skip newline
      }
      // const prevBuffer = buffer;
      buffer = buffer.slice(pos);
      // console.log({data, buffer, prevBuffer});
    }

    port.on('data', function(data) {
      try {
        handleData(data);
      } catch (err) {
        console.error(err);
      }
    });
  }

  sendCommand(cmd, data) {
    const msg = Buffer.alloc(512);

    msg[0] = 'C'.charCodeAt(0);
    msg[1] = 'M'.charCodeAt(0);
    msg[2] = 'D'.charCodeAt(0);
    msg[3] = cmd.charCodeAt(0);
    if (data != null) {
      msg.writeUInt32BE(data, 5);
    }
    console.log('sending', trimNullBytes(msg));
    port.write(msg);
  }
}
