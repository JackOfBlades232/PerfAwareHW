#!/bin/bash

pushd build
clang++ -g -std=c++20 -mfma -mavx2 -I ../../Common/ -I ../Components/ ../Generator/*.cpp $@ -o generate
clang++ -g -std=c++20 -mfma -mavx2 -I ../../Common/ -I ../Components/ -Wno-return-local-addr ../Solver/*.cpp $@ -o solve
popd
