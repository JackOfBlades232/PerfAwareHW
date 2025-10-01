#!/bin/bash

pushd build
g++ -g -std=c++20 -Wno-format -I ../../Common/ -I ../../Haversine/Components/ ../haversine_perf_test.cpp $@ -o haversine_perf_test
popd

