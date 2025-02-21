#!/bin/bash

pushd build
g++ -g -std=c++20 -I ../../Common/ ../fileread.cpp $@ -o fileread
g++ -g -std=c++20 -I ../../Common/ ../pagefaults.cpp $@ -o pagefaults
nasm -f elf64 ../bandwidth_funcs.asm
nasm -f elf64 ../branch_funcs.asm
nasm -f elf64 ../calign_funcs.asm
nasm -f elf64 ../exprt_funcs.asm
mv ../bandwidth_funcs.o ./
mv ../branch_funcs.o ./
mv ../calign_funcs.o ./
mv ../exprt_funcs.o ./
g++ -g -std=c++20 -I ../../Common/ ../bandwidth.cpp bandwidth_funcs.o $@ -o bandwidth
g++ -g -std=c++20 -I ../../Common/ ../branch.cpp branch_funcs.o $@ -o branch
g++ -g -std=c++20 -I ../../Common/ ../calign.cpp calign_funcs.o $@ -o calign
g++ -g -std=c++20 -I ../../Common/ ../exprt.cpp exprt_funcs.o $@ -o exprt
popd
