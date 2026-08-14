@echo off
pushd build
clang-cl /Zi /std:c++20 ..\*.cpp /Fe: main.exe
popd
@echo on
