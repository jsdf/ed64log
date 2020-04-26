# Example app for ed64log

This is a demo app which shows how to use ed64log in an N64 gmae. The game-specific code is in [stage00.c](stage00.c). If you press the A or B buttons, text will be logged.

If you press the C-left or C-right button the app will intentionally crash, printing debug information to ed64log. You can then copy the program counter value (called `epc`) and pass it to the `./disassemble.sh` script, eg.:

```bash
./disassemble.sh 0x80025ea0
```

This will output disassembly around the program counter, including the current function name:

![disassembly screenshot](disassembly.png)
 
## How to build (macOS or linux):

If you already know how to build N64 roms, this is a standard SDK application using NuSystem, so build in in the usual way with `make`.


- Install [wine](https://www.winehq.org/)
- Install the n64 sdk. this repo assumes it's in the default location of `C:\ultra` (in the wine filesystem). If you've installed it somewhere else, you'll need to update this path in `compile.bat`
- Edit `./build.sh` so that WINE_PATH points to your wine binary
- Run `./build.sh`. This should build ed64logdemo.n64
- Run ed64logdemo.n64 with an emulator or an N64 flashcart

