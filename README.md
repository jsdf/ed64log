# ed64log

enables printf() logging over usb from homebrew games built using the n64 sdk, while running on an everdrive 64 device.


### getting started

to implement usb logging in your n64 game:

- copy the sources from the [n64](n64) directory of this repo into your game's source, and add the .c files to your Makefile.
  this code depends on some library functions provided by NuSystem, so make sure you have configured your Makefile to include the NuSystem library, as described in the n64 sdk docs (eg. it should include `-lnusys` or `-lnusys_d`).


- in the initialization of your game, include [ed64io_everdrive.h](n64/ed64io_everdrive.h) and call `evd_init()` to initialize the everdrive hardware for writing.
- in the main loop (or at some regular interval) include [ed64io_usb.h](n64/ed64io_usb.h) and call `usbLoggerFlush()` to write pending logs to the computer via usb.
- to use printf logging in your game, include [ed64io_usb.h](n64/ed64io_usb.h) in your source file and use `ed64Printf()` as you would normally use `printf()` in other programs.

you can see an example of implementing logging in a game in this commit to [goose64](https://github.com/jsdf/goose64/commit/cf2259a2b47cd8e2f828ad61a5dd5ddcd2c02986).


### using the tool (macOS)

install required libraries from homebrew:

```bash
brew install libftdi libusb
```

- in XCode, open [xcode/ed64log.xcodeproj](xcode/ed64log.xcodeproj)

- by default, building will install ed64log to `/usr/local/bin`. If you want to change this, in the ed64log target's build phases tab, change the 'copy files' command output path to where you want the ed64log binary installed

- click 'run' to build and install the ed64log binary

then, after starting the game on the everdrive, run

```bash
./ed64log
```

this will output the logs as they are sent from the everdrive


### using the tool (linux)

install libftdi from your package manager of choice

compile the ed64log tool from source:

```
chmod +x make.sh
./make.sh
```

then, to start tailing the logs:

```
sudo ./ed64log
```


### using the tool (windows)

install required libraries from homebrew:

```bash
brew install libftdi libusb
```

- in XCode, open [xcode/ed64log.xcodeproj](xcode/ed64log.xcodeproj)

- by default, building will install ed64log to `/usr/local/bin`. If you want to change this, in the ed64log target's build phases tab, change the 'copy files' command output path to where you want the ed64log binary installed

- click 'run' to build and install the ed64log binary

then, after starting the game on the everdrive, run

```bash
./ed64log
```

this will output the logs as they are sent from the everdrive


### acknowledgements

part of the source is based on [loader64](http://krikzz.com/forum/index.php?topic=1407.msg14076) by saturnu <tt@anpa.nl>