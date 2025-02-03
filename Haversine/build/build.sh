#!/bin/bash

pushd build
g++ -g -std=c++20 -I ../../Common/ ../Generator/*.cpp $@ -o generate
g++ -g -std=c++20 -I ../../Common/ -Wno-return-local-addr ../Solver/*.cpp $@ -o solve
popd
