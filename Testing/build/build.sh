#!/bin/bash

pushd build
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../fileread.cpp $@ -o fileread
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../pagefaults.cpp $@ -o pagefaults
nasm -f elf64 ../bandwidth_funcs.asm
nasm -f elf64 ../branch_funcs.asm
nasm -f elf64 ../calign_funcs.asm
nasm -f elf64 ../exprt_funcs.asm
nasm -f elf64 ../bandwidth_nocache_funcs.asm
nasm -f elf64 ../cache_bandwidth_funcs.asm
mv ../bandwidth_funcs.o ./
mv ../branch_funcs.o ./
mv ../calign_funcs.o ./
mv ../exprt_funcs.o ./
mv ../bandwidth_nocache_funcs.o ./
mv ../cache_bandwidth_funcs.o ./
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../bandwidth.cpp bandwidth_funcs.o $@ -o bandwidth
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../branch.cpp branch_funcs.o $@ -o branch
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../calign.cpp calign_funcs.o $@ -o calign
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../exprt.cpp exprt_funcs.o $@ -o exprt
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../bandwidth_nocache.cpp bandwidth_nocache_funcs.o $@ -o bandwidth_nocache
g++ -g -std=c++20 -Wno-format -I ../../Common/ ../cache_bandwidth.cpp cache_bandwidth_funcs.o $@ -o cache_bandwidth
popd
