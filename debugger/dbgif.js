#!/usr/bin/env node
const SerialPort = require('serialport');
const ByteLength = require('@serialport/parser-byte-length');

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

const PACKET_TYPES = {
  NonePacket: 0,
  RegistersPacket: 1,
  ThreadPacket: 2,
};
const THREAD_STATE_NAMES = [
  'stopped',
  'runnable',
  'running',
  'waiting',
  'invalid',
];

class DebuggerInterface extends EventEmitter {
  port = null;

  async getSerialPort() {
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

    this.attachParser(port);

    port.on('error', function(err) {
      console.error(err);
    });

    let exiting = false;
    process.on('SIGINT', function() {
      console.log('exiting...');
      if (exiting) {
        return;
      }
      exiting = true;

      setTimeout(() => {
        console.log('took too long to exit safely, exiting unsafely');
        process.exit(1);
      }, 3000);

      port.pause();
      port.drain((err) => {
        if (err) console.error(err);

        port.close((err) => {
          if (err) console.error(err);
          process.exit(0);
        });
      });
    });

    return port;
  }

  async start() {
    if (this.port) {
      throw new Error('already started DebuggerInterface');
    }
    this.port = await this.getSerialPort();
    if (!this.port) {
      throw new Error('failed to get port');
    } else {
      console.log('connected to everdrive64 on', this.port.path);
    }
  }

  attachParser(port) {
    let buffer = Buffer.alloc(0);

    const visitReceivedLine = (line) => {
      // here we can intercept in-band signalling
      if (line.startsWith('EDBG')) {
        if (line.startsWith('EDBG=break')) {
          const [msgType, id] = line.split(' ');
          console.log('break', id);
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
          // this.emit('threadstate', {
          //   id,
          //   stateName,
          //   priority,
          //   pc,
          //   stacktrace: [...new Set(stacktrace)],
          // });
        }
      } else if (line.startsWith('TRACE=')) {
        this.emit('trace', line);
      } else {
        this.emit('log', line);
      }

      if (process.env.NODE_ENV === 'development') {
        console.log(line);
      }
    };

    const visitBinaryPacket = (packet) => {
      let pos = 0;
      const packetType = packet.readUInt16BE(pos);
      pos += 2;
      const length = packet.readUInt16BE(pos);
      pos += 2;
      // console.log('visitBinaryPacket', {packetType, length});
      switch (packetType) {
        case PACKET_TYPES.ThreadPacket:
          const threadState = {};
          ['id', 'stateName', 'priority', 'pc', 'ra'].forEach((field) => {
            const value = packet.readUInt32BE(pos);

            switch (field) {
              case 'id':
              case 'priority':
                threadState[field] = value.toString();
                break;
              case 'stateName':
                threadState[field] = THREAD_STATE_NAMES[value] || 'invalid';
                break;
              case 'pc':
              case 'ra':
                threadState[field] = value.toString(16);
                break;
            }
            // console.log(field, {
            //   pos,
            //   parsed: threadState[field],
            //   raw: packet.slice(pos, pos + 4),
            // });
            pos += 4;
          });
          threadState.stacktrace = [];
          let i;
          for (i = 0; i < 100; i++) {
            if (pos >= length) {
              // if we get to the end of the packet, we can't read any further
              break;
            }
            const stackframe = packet.readUInt32BE(pos);
            pos += 4;
            if (stackframe === 0) {
              break;
            }
            threadState.stacktrace[i] = stackframe.toString(16);
          }
          threadState.stacktrace.length = i + 1;
          threadState.stacktrace = [
            ...new Set(
              [
                threadState.pc,
                threadState.ra,
                ...threadState.stacktrace,
              ].filter(Boolean)
            ),
          ];
          // console.log('got binary threadstate', threadState);
          this.emit('threadstate', threadState);
      }
    };

    function handleData(data) {
      if (data.slice(0, 4).toString() === '\0bin') {
        // console.log('got binary packet', [...data].map((v) => v.toString(16)));
        visitBinaryPacket(data.slice(4));
        return;
      }
      if (data.toString().includes('\0bin')) {
        console.error('got binary packet but the header is not at the start');
      }
      // console.log(
      //   `chunk length=${data.length}`,
      //   [...data].map((v) => v.toString(16))
      // );
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

    const packetStream = new ByteLength({length: 512});

    packetStream.on('data', function(data) {
      try {
        handleData(data);
      } catch (err) {
        console.error(err);
      }
    });

    port.pipe(packetStream);
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
    if (!this.port) {
      console.error('port not connected', this);
      return;
    }
    this.port.write(msg);
  }
}

module.exports = DebuggerInterface;
