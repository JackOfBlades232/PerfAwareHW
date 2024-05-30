@echo off
pushd build
cl /Zi /std:c++20 ..\decoder8086.cpp
popd
@echo on
