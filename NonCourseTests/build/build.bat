pushd build
clang-cl /Zi /arch:AVX2 /Oi /O2 %* /std:c++20 /I..\..\Common ..\cbrt_perf_test.cpp /Fe: cbrt_perf_test.exe
clang-cl /Zi /arch:AVX2 /Oi /O2 %* /std:c++20 /I..\..\Common ..\scaling_factor_perf_test.cpp /Fe: scaling_factor_perf_test.exe
popd
