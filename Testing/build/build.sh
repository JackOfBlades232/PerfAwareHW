#!/bin/bash

pushd build
g++ -g -std=c++20 -I ../../Common/ ../*.cpp $@ -o run
popd
