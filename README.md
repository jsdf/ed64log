# loader64
loader64 - Everdrive64 USB-tool v0.1

OS64 compatible USB upload tool

by saturnu <tt@anpa.nl>

originally from http://krikzz.com/forum/index.php?topic=1407.msg14076

forked by @jsdf to add macOS compatibility

## macOS instructions

### building from source

install required libraries from homebrew:

```bash
brew install libftdi libusb
```

- in XCode, open [xcode/loader64.xcodeproj](xcode/loader64.xcodeproj)

- by default, building will install loader64 to `/usr/local/bin`. If you want to change this, in the loader64 target's build phases tab, change the 'copy files' command output path to where you want the loader64 binary installed

- click 'run' to build and install loader64 binary

### usage

uploading and booting a rom:

```bash
# upload rom
./loader64 -v --write --file somerom.n64

# boot rom that was uploaded
./loader64 -v --pifboot
```


## linux instructions
please run as root


required libs:
libftdi 0.20
http://www.intra2net.com/en/developer/libftdi/


compilation:

```
chmod +x make.sh
./make.sh
```

- examples -

upload rom:

```
sudo ./loader64 -vwf OS64.v64
```

boot rom:
```
sudo ./loader64 -vp
```
