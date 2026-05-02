#!/bin/bash

pushd build
nasm -f elf64 ../dependency_chains_funcs.asm
mv ../dependency_chains_funcs.o ./
clang++-21 $@ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_perf_test.cpp -o haversine_perf_test
clang++-21 $@ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_input_ranges.cpp -o haversine_input_ranges
clang++-21 $@ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_func_precision_test.cpp -o haversine_func_precision_test
clang++-21 $@ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../benchmark_method_eval.cpp -o benchmark_method_eval
clang++-21 $@ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../dependency_chains.cpp dependency_chains_funcs.o -o dependency_chains
popd

