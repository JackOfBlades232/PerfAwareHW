#!/bin/bash

pushd build
g++ -g -std=c++20 -I ../../Common/ ../fileread.cpp $@ -o fileread
g++ -g -std=c++20 -I ../../Common/ ../pagefaults.cpp $@ -o pagefaults
popd
