#!/bin/bash

pushd build
g++ -g -std=c++20 -I ../../Common/ ../fileread.cpp $@ -o fileread
g++ -g -std=c++20 -I ../../Common/ ../pagefaults.cpp $@ -o pagefaults
nasm -f elf64 ../bandwidth_funcs.asm
mv ../bandwidth_funcs.o ./
g++ -g -std=c++20 -I ../../Common/ ../bandwidth.cpp bandwidth_funcs.o $@ -o bandwidth
popd
