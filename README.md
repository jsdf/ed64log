# ed64log

based on loader64 - Everdrive64 USB-tool v0.1
by saturnu <tt@anpa.nl>



### building from source

install required libraries from homebrew:

```bash
brew install libftdi libusb
```

- in XCode, open [xcode/ed64log.xcodeproj](xcode/ed64log.xcodeproj)

- by default, building will install ed64log to `/usr/local/bin`. If you want to change this, in the ed64log target's build phases tab, change the 'copy files' command output path to where you want the ed64log binary installed

- click 'run' to build and install ed64log binary

### usage

[implement usb logging in your n64 game](https://github.com/jsdf/goose64/commit/cf2259a2b47cd8e2f828ad61a5dd5ddcd2c02986)

then, after starting the game on the everdrive, run

```bash
./ed64log
```

this will output the logs as they are sent from the everdrive


