@echo off
pushd build
cl /Zi /std:c++20 ..\PerfAwareHW.cpp
popd
@echo on
