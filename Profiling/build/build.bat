@echo off
pushd build
@REM cl /Zi /std:c++20 ..\Generator\*.cpp /Fe: generate.exe
cl /Zi /std:c++20 ..\Solver\*.cpp /Fe: solve.exe
popd
@echo on
