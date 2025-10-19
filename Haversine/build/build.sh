#!/bin/bash

pushd build
g++ -g -std=c++20 -mfma -mavx2 -I ../../Common/ -I ../Components/ ../Generator/*.cpp $@ -o generate
g++ -g -std=c++20 -mfma -mavx2 -I ../../Common/ -I ../Components/ -Wno-return-local-addr ../Solver/*.cpp $@ -o solve
popd
