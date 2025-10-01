#!/bin/bash

pushd build
cl /Zi /std:c++20 /I..\..\Common /I..\..\Haversine\Components ..\haversine_perf_test.cpp %* /Fe: haversine_perf_test.exe
popd

