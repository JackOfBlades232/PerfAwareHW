#!/bin/bash

pushd build
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\dependency_chains_funcs.asm
move ..\dependency_chains_funcs.obj .\
clang-cl /Zi /arch:AVX2 /clang:-masm=intel /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_perf_test.cpp /Fe: haversine_perf_test.exe
clang-cl /Zi /arch:AVX2 /clang:-masm=intel /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_input_ranges.cpp /Fe: haversine_input_ranges.exe
clang-cl /Zi /arch:AVX2 /clang:-masm=intel /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_func_precision_test.cpp /Fe: haversine_func_precision_test.exe
clang-cl /Zi /arch:AVX2 /clang:-masm=intel /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\benchmark_method_eval.cpp /Fe: benchmark_method_eval.exe
clang-cl /Zi /arch:AVX2 /clang:-masm=intel /Oi %* /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\dependency_chains.cpp dependency_chains_funcs.obj /Fe: dependency_chains.exe
popd
