@echo off
pushd build
cl /Zi /std:c++20 ..\decoder.cpp
popd
@echo on
