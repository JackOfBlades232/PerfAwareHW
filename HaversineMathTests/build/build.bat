#!/bin/bash

pushd build
cl /Zi /Oi /std:c++20 %* /I..\..\Common /I..\..\Haversine\Components ..\haversine_perf_test.cpp /Fe: haversine_perf_test.exe
cl /Zi /Oi /std:c++20 %* /I..\..\Common /I..\..\Haversine\Components ..\haversine_input_ranges.cpp /Fe: haversine_input_ranges.exe
cl /Zi /Oi /std:c++20 %* /I..\..\Common /I..\..\Haversine\Components ..\haversine_func_precision_test.cpp /Fe: haversine_func_precision_test.exe
popd

