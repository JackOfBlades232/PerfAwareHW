@echo off
pushd build
cl /Zi /std:c++20 /I..\..\Common ..\fileread.cpp %* /Fe: fileread.exe
cl /Zi /std:c++20 /I..\..\Common ..\pagefaults.cpp %* /Fe: pagefaults.exe
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\bandwidth_funcs.asm
move ..\bandwidth_funcs.obj .\
cl /Zi /std:c++20 /I..\..\Common ..\bandwidth.cpp bandwidth_funcs.obj %* /Fe: bandwidth.exe
popd
@echo on
