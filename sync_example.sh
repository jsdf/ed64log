#!/bin/bash

# sync changes from example codebase back to source dir
rm -f n64/*.*
cp example/ed64io*.{c,h} n64/
