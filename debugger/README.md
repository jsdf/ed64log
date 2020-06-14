# ed64 debugger


```
# in this directory, run
npm install # just once after cloning this repo

# start the debugger for a particular game binary
OBJDUMP_PATH="mips64-elf-objdump" APP_BINARY_PATH="/path/to/binary.out" npm start
```


## development instructions

```
# in example/ dir
DEBUGGER=1 ./build.sh && DEBUGGER=1 ./deploy.sh
```
