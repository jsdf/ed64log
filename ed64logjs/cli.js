#!/usr/bin/env node

const DebuggerInterface = require('./dbgif');

const dbgif = new DebuggerInterface();
dbgif.start();

dbgif.on('trace', (line) => {
  console.log(`TRACE=[${line.length - 6} bytes]`);
});
dbgif.on('log', (line) => {
  process.stdout.write(line + '\n');
});
