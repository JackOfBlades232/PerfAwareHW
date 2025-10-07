@echo off
pushd build
cl /Zi /arch:AVX2 /std:c++20 /I..\..\Common /I..\Components ..\Generator\*.cpp %* /Fe: generate.exe
cl /Zi /arch:AVX2 /std:c++20 /I..\..\Common /I..\Components /wd4172 ..\Solver\*.cpp %* /Fe: solve.exe
popd
@echo on
