@echo off
pushd build
cl /Zi /std:c++20 ..\Generator\*.cpp /Fe: generate.exe
popd
@echo on
