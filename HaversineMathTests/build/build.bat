#!/bin/bash

pushd build
cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_perf_test.cpp /Fe: haversine_perf_test.exe
cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_input_ranges.cpp /Fe: haversine_input_ranges.exe
cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_func_precision_test.cpp /Fe: haversine_func_precision_test.exe
popd
