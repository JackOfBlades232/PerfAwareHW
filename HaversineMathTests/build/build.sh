#!/bin/bash

pushd build
g++ -g -std=c++20 -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_perf_test.cpp $@ -o haversine_perf_test
g++ -g -std=c++20 -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_input_ranges.cpp $@ -o haversine_input_ranges
g++ -g -std=c++20 -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_func_precision_test.cpp $@ -o haversine_func_precision_test
popd

