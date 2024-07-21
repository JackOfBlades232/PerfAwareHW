@echo off
..\build\main.exe sim %1 -o test_sim.txt -trace %2 %3 %4 %5 && fc test_sim.txt %1.txt
@echo on
