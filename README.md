# loader64
loader64 - Everdrive64 USB-tool v0.1

OS64 compatible USB upload tool

by saturnu <tt@anpa.nl>

originally from http://krikzz.com/forum/index.php?topic=1407.msg14076

forked by @jsdf to add macOS compatibility

## macOS instructions

install libraries:

```bash
brew install libftdi libusb
```

in XCode, open loader64/loader64.xcodeproj

in the loader64 target's build phases tab, change the 'copy files' command output path to where you want the binary installed



example of uploading and booting a rom:

```bash
# upload rom
./loader64 -v --write --file somerom.n64

# boot rom that was uploaded
./loader64 -v --pifboot
```


## original linux instructions
please run as root


required libs:
libftdi 0.20
http://www.intra2net.com/en/developer/libftdi/


compilation:
chmod +x make.sh
./make.sh


- examples -

upload rom:
sudo ./loader64 -vwf OS64.v64

boot rom:
sudo ./loader64 -vp
