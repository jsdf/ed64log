# ed64log

enables printf() logging over usb from homebrew games built using the n64 sdk, while running on an everdrive 64 device.


### getting started

to implement usb logging in your n64 game:

- copy the sources from the [n64](n64) directory of this repo into your game's source, and add the .c files to your Makefile.

- in the initialization of your game, include [ed64io_everdrive.h](n64/ed64io_everdrive.h) and call `evd_init()` to initialize the everdrive hardware for writing.

#### synchronous logging
- include [ed64io_usb.h](n64/ed64io_usb.h) in your source file and use `ed64PrintfSync()` as you would normally use `printf()` in other programs. note: this will block the thread (eg. temporarily freeze the game) until it finishes logging. this is mainly useful to guarantee that the log message gets through, eg. before a crash/failed assertion, or to test that the logging functionality is working.

#### asynchronous logging
- in the main loop (or at some regular interval) include [ed64io_usb.h](n64/ed64io_usb.h) and call `ed64AsyncLoggerFlush()` to write pending logs to the computer via usb.
- to use printf logging in your game, include [ed64io_usb.h](n64/ed64io_usb.h) in your source file and use `ed64Printf()` as you would normally use `printf()` in other programs.
- up to 512 chars will be buffered. if you log more than that without calling ed64AsyncLoggerFlush, the output will be truncated. if you want to log a lot you wil need to ed64AsyncLoggerFlush() more frequently during a frame. you could even call it on a timer in a background thread. i haven't explored this fully.

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

install `libftdi` from your package manager of choice (including headers). eg for ubuntu:

```bash
sudo apt-get install libftdi1 libftdi1-dev
```

compile the ed64log tool from source:

```bash
./make.sh
```

then, to start tailing the logs:

```bash
# needs sudo to access usb device
sudo ./ed64log
```


### using the tool (windows) (experimental) (who knows if it works)
- download the latest [win32 binary](https://jamesfriend.com.au/projects/n64dev/ed64log-win32-latest.zip) or [win64 binary](https://jamesfriend.com.au/projects/n64dev/ed64log-win64-latest.zip).

or

- install gcc (eg. with mingw)
- clone this repo
- download and extract [this](https://sourceforge.net/projects/picusb/files/libftdi1-1.4git_devkit_x86_x64_14June2018.zip/download) to `../libftdi` (adjacent to your clone of this repo)
- run `make.bat` in this repo


then, after starting the game on the everdrive, run

```bash
ed64log.exe
```

this will output the logs as they are sent from the everdrive


### acknowledgements

part of the source is based on [loader64](http://krikzz.com/forum/index.php?topic=1407.msg14076) by saturnu <tt@anpa.nl>
