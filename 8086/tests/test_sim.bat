@echo off
..\build\main.exe sim %1 -o test_sim.txt -trace && fc test_sim.txt %1.txt
@echo on
