#!/usr/bin/env bash

rm -rf bin Release cmake-build-debug &> /dev/null
mkdir Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..

BIN_DIR=bin
BIN_NAME=code_generator
#chmod 777 $BIN_DIR
chmod 777 $BIN_DIR/$BIN_NAME

