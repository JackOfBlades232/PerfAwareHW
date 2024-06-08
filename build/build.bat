@echo off
pushd build
cl /Zi /std:c++20 ..\*.cpp /Fe: main.exe
popd
@echo on
