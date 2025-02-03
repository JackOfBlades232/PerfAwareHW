@echo off
pushd build
cl /Zi /std:c++20 /I..\..\Common ..\*.cpp %* /Fe: run.exe
popd
@echo on
