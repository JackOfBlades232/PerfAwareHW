@echo off
..\build\main.exe disasm %1 -o test_disasm.asm && nasm.exe test_disasm.asm && fc test_disasm %1
@echo on
