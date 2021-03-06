const readline = require('readline');

const DebuggerInterface = require('../node-ed64log/dbgif');

const dbgif = new DebuggerInterface();
dbgif.start();

dbgif.on('break', () => {
  breakpointPrompt(dbgif);
});
dbgif.on('trace', (line) => {
  console.log(`TRACE=[${line.length - 6} bytes]`);
});
dbgif.on('log', (line) => {
  process.stdout.write(line + '\n');
});

function parseAndSendCommand(dbgif, cmdText) {
  const cmdType = cmdText.trim()[0];
  switch (cmdType) {
    case 'r':
    case 's':
      dbgif.sendCommand(cmdType);
      return true;
    case 'b': {
      const address = parseInt(cmdText.trim().split(' ')[1], 16);
      if (!isNaN(address)) {
        dbgif.sendCommand(cmdType, address);
        return true;
      } else {
        console.log('failed to parse breakpoint');
      }
    }
  }
  return false;
}

function breakpointPrompt(dbgif) {
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });

  rl.question(
    `
  hit breakpoint. enter a command:
  r: run
  s: step
  b [address]: set breakpoint at [address]
  `,
    function(cmdText) {
      rl.close();
      if (!parseAndSendCommand(dbgif, cmdText)) {
        breakpointPrompt(dbgif);
      }
    }
  );
}
