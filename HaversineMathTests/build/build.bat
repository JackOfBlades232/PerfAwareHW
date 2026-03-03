#!/bin/bash

pushd build
clang-cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_perf_test.cpp /Fe: haversine_perf_test.exe
clang-cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_input_ranges.cpp /Fe: haversine_input_ranges.exe
clang-cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_func_precision_test.cpp /Fe: haversine_func_precision_test.exe
clang-cl /Zi /arch:AVX2 /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\benchmark_method_eval.cpp /Fe: benchmark_method_eval.exe
popd
