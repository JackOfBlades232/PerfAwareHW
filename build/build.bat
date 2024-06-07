@echo off
pushd build
cl /Zi /std:c++20 ..\*.cpp
popd
@echo on
