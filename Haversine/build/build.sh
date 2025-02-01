#!/bin/bash

pushd build
g++ -g -std=c++20 ../Generator/*.cpp $@ -o generate
g++ -g -std=c++20 -Wno-return-local-addr ../Solver/*.cpp $@ -o solve
popd
