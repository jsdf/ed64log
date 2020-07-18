#!/bin/bash
set -eu

rm -f *.o

SDK_BASE_DIR="/Users/jfriend/.wine/drive_c"
export ROOT="${SDK_BASE_DIR}/ultra"
export N64KITDIR="${SDK_BASE_DIR}/nintendo/n64kit"

make
