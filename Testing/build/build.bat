@echo off
pushd build
cl /Zi /std:c++20 /I..\..\Common ..\fileread.cpp %* /Fe: fileread.exe
cl /Zi /std:c++20 /I..\..\Common ..\pagefaults.cpp %* /Fe: pagefaults.exe
cl /Zi /std:c++20 /I..\..\Common ..\bandwidth.cpp %* /Fe: bandwidth.exe
popd
@echo on
