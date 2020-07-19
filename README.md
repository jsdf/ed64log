# ed64log

enables printf() logging over usb from homebrew games built using the n64 sdk, while running on an everdrive 64 device.


### getting started

at a high level, the steps involved are:

- copy the code to your n64 game and add calls to the ed64log api
- install or build the logger client program on your computer

then to run your game with logging:

- connect to the everdrive via usb
- build and run your game on the everdrive
- run the logger client program on your computer

### implement usb logging in your n64 game

- copy the sources from the [n64](n64) directory of this repo into your game's source, and add the .c files to your Makefile.

- in the initialization of your game, include [ed64io.h](n64/ed64io.h) and call `evd_init()` to initialize the everdrive hardware for writing. see the [example app](https://github.com/jsdf/ed64log/blob/master/example/main.c).

- then add synchronous or asynchronous logging calls into your game code

#### synchronous logging
- include [ed64io.h](n64/ed64io.h) in your source file and use `ed64PrintfSync()` as you would normally use `printf()` in other programs. note: this will block the thread (eg. temporarily freeze the game) until it finishes logging (including if the host is not connected). this is mainly useful to guarantee that the log message gets through, eg. before a crash/failed assertion, or to test that the logging functionality is working.

#### asynchronous logging
- in the main loop (or at some regular interval) include [ed64io.h](n64/ed64io.h) and call `ed64AsyncLoggerFlush()` to write pending logs to the computer via usb.
- to use printf logging in your game, include [ed64io.h](n64/ed64io.h) in your source file and use `ed64Printf()` as you would normally use `printf()` in other programs.
- up to 512 chars will be buffered. if you log more than that without calling `ed64AsyncLoggerFlush`, the output will be truncated. if you want to log a lot you wil need to `ed64AsyncLoggerFlush()` more frequently during a frame. you could even call it on a timer in a background thread. i haven't explored this fully.

you can see an example of implementing logging in a game in this commit to [goose64](https://github.com/jsdf/goose64/commit/cf2259a2b47cd8e2f828ad61a5dd5ddcd2c02986).

#### osSyncPrintf support

you can override nintendo's implementation of `osSyncPrintf` with an ed64log-based version by calling `ed64ReplaceOSSyncPrintf()` once as part of your game's startup process. this will allow all `osSyncPrintf()` calls to work, including debug error messages in libultra_d and libmus_d. 

#### exception logger

optionally, you can set up an exception handler thread to automatically log details when an exception occurs (eg. invalid memory access) by calling `ed64StartFaultHandlerThread()`, passing the thread priority to run at (typically, the same as the main thread) [like this](https://github.com/jsdf/ed64log/blob/master/example/main.c#L17). for usage details, see the [example app readme](https://github.com/jsdf/ed64log/tree/master/example#exception-logging-and-disassembly).

#### OS error logger

optionally, you can override the default N64 OS error handler which prints errors in calls to libultra functions to `osSyncPrintf`, with one that prints to `ed64SyncPrintf` instead, by calling `ed64RegisterOSErrorHandler()` once in the initialization of your progam.

**note:** this is not needed if you override `osSyncPrintf()` as described above.

#### watchdog timer

if you experience a freeze/hang in your game which doesn't produce any useful error message, you can create a watchdog timer to detect when the game has frozen and then print out useful information about the application state.

`ed64StartWatchdogThread` takes a pointer to an integer value which should continually change while the program is working correctly (eg. a counter of the absolute frame number) and an interval in milliseconds at which the watchdog will check if the program is still running. when the watchdog timer interrupts the program it will check if the current value of the integer has changed since the last check. if the program has frozen (for example, having entered an infinite loop), the watchdog thread will print the current state of each thread, and a stacktrace of the thread's execution.

example:

```c
ed64StartWatchdogThread(&intValueThatIncrements, 500);
```



### connecting from your computer (all platforms, node.js based)

on any OS, you can use the node.js-based client to receive logs

first install [node and npm](https://nodejs.org/)

then install ed64logjs:

```
npm install -g ed64logjs
```

then, after starting the game on the everdrive, run

```bash
ed64logjs
```



### connecting from your computer (macOS native)

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


### connecting from your computer (linux native)

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


### acknowledgements

the source of the ed64log client is based on [loader64](http://krikzz.com/forum/index.php?topic=1407.msg14076) by saturnu <tt@anpa.nl>

