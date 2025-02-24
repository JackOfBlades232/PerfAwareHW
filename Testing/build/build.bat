@echo off
pushd build
cl /Zi /std:c++20 /I..\..\Common ..\fileread.cpp %* /Fe: fileread.exe
cl /Zi /std:c++20 /I..\..\Common ..\pagefaults.cpp %* /Fe: pagefaults.exe
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\bandwidth_funcs.asm
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\branch_funcs.asm
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\calign_funcs.asm
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\exprt_funcs.asm
"C:\Program Files\NASM\nasm.exe" -f win64 -D_WIN32=1 ..\bandwidth_nocache_funcs.asm
move ..\bandwidth_funcs.obj .\
move ..\branch_funcs.obj .\
move ..\calign_funcs.obj .\
move ..\exprt_funcs.obj .\
move ..\bandwidth_nocache_funcs.obj .\
cl /Zi /std:c++20 /I..\..\Common ..\bandwidth.cpp bandwidth_funcs.obj %* /Fe: bandwidth.exe
cl /Zi /std:c++20 /I..\..\Common ..\branch.cpp branch_funcs.obj %* /Fe: branch.exe
cl /Zi /std:c++20 /I..\..\Common ..\calign.cpp calign_funcs.obj %* /Fe: calign.exe
cl /Zi /std:c++20 /I..\..\Common ..\exprt.cpp exprt_funcs.obj %* /Fe: exprt.exe
cl /Zi /std:c++20 /I..\..\Common ..\bandwidth_nocache.cpp bandwidth_nocache_funcs.obj %* /Fe: bandwidth_nocache.exe
popd
@echo on
