@echo off
..\build\main.exe disasm listing_0042_completionist_decode -o test_disasm.asm && nasm.exe test_disasm.asm && fc test_disasm listing_0042_completionist_decode
@echo on
