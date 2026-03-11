#!/bin/bash

pushd build
clang++ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_perf_test.cpp $@ -o haversine_perf_test
clang++ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_input_ranges.cpp $@ -o haversine_input_ranges
clang++ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_func_precision_test.cpp $@ -o haversine_func_precision_test
clang++ -g -std=c++20 -mfma -mavx2 -masm=intel -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../benchmark_method_eval.cpp $@ -o benchmark_method_eval
clang++ -g -std=c++20 -mfma -mavx2 -masm=intel -fno-vectorize -fno-slp-vectorize -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../dependency_chains.cpp $@ -o dependency_chains
popd

