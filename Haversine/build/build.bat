@echo off
pushd build
cl /Zi /std:c++20 /I..\..\Common ..\Generator\*.cpp %* /Fe: generate.exe
cl /Zi /std:c++20 /I..\..\Common /wd4172 ..\Solver\*.cpp %* /Fe: solve.exe
popd
@echo on
