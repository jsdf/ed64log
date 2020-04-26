# Example app for ed64log

This is a demo app which shows how to use ed64log in an N64 game. The game-specific code is in [stage00.c](stage00.c). If you press the A or B buttons, text will be logged.

If you press the C-left or C-right button the app will intentionally crash, printing debug information to ed64log, eg:

```
Fault in thread 4:

epc   0x80025ea0
cause   0x00000010 <Address error on load or instruction fetch>
sr    0x2000ff03 <CU1,IM8,IM7,IM6,IM5,IM4,IM3,IM2,IM1,KER,EXL,IE>
badvaddr  0x00000001

register contents:
at 0xffffffff80050000 v0 0x0000000000000044 v1 0x0000000000000000
a0 0x0000000000000000 a1 0x00000000003d245b a2 0x0000000000000000
a3 0x0000000000000000 t0 0x00000000003d0f84 t1 0x0000000000000a98
t2 0x0000000000000000 t3 0x000000000000ff00 t4 0x0000000000000000
t5 0xffffffff8003cb38 t6 0x0000000000000000 t7 0x0000000000000000
s0 0x0000000000000000 s1 0x0000000000000000 s2 0x0000000000000000
s3 0x0000000000000000 s4 0x0000000000000000 s5 0x0000000000000000
s6 0x0000000000000000 s7 0x0000000000000000 t8 0x0000000000000000
t9 0x00000000000fffff gp 0x0000000000000000 sp 0xffffffff800e1578
ra 0xffffffff80025ea0

fpcsr   0x01000804 <FS,EV,FI,RN>
```

You can then copy the program counter value (called `epc`) and pass it to the `./disassemble.sh` script, eg.:

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
