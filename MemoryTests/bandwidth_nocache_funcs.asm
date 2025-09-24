; all is 2x cause I know i have 2 load and 2 store ports
global run_loop_load2x1
global run_loop_load2x2
global run_loop_load2x4
global run_loop_load2x8
global run_loop_load2x16
global run_loop_load2x32
global run_loop_load2x64
global run_loop_store2x1
global run_loop_store2x2
global run_loop_store2x4
global run_loop_store2x8
global run_loop_store2x16
global run_loop_store2x32
global run_loop_store2x64

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%else
%define PARAM0 rdi
%define PARAM1 rsi
%endif

section .text

run_loop_load2x1:
                mov     rax, PARAM0
.loop:          mov     r8b, [PARAM1]
                mov     r9b, [PARAM1 + 1]
                sub     PARAM0, 2
                ja      .loop
                ret

run_loop_load2x2:
                mov     rax, PARAM0
.loop:          mov     r8w, [PARAM1]
                mov     r9w, [PARAM1 + 2]
                sub     PARAM0, 4
                ja      .loop
                ret

run_loop_load2x4:
                mov     rax, PARAM0
.loop:          mov     r8d, [PARAM1]
                mov     r9d, [PARAM1 + 4]
                sub     PARAM0, 8
                ja      .loop
                ret

run_loop_load2x8:
                mov     rax, PARAM0
.loop:          mov     r8, [PARAM1]
                mov     r9, [PARAM1 + 8]
                sub     PARAM0, 16
                ja      .loop
                ret

run_loop_load2x16:
                mov     rax, PARAM0
.loop:          vmovups xmm0, [PARAM1]
                vmovups xmm1, [PARAM1 + 16]
                sub     PARAM0, 32
                ja      .loop
                ret

run_loop_load2x32:
                mov     rax, PARAM0
.loop:          vmovups ymm0, [PARAM1]
                vmovups ymm1, [PARAM1 + 32]
                sub     PARAM0, 64
                ja      .loop
                ret

run_loop_load2x64:
                mov     rax, PARAM0
.loop:          vmovups zmm0, [PARAM1]
                vmovups zmm1, [PARAM1 + 64]
                sub     PARAM0, 128
                ja      .loop
                ret

run_loop_store2x1:
                mov     rax, PARAM0
.loop:          mov     [PARAM1], r8b
                mov     [PARAM1 + 1], r9b
                sub     PARAM0, 2
                ja      .loop
                ret

run_loop_store2x2:
                mov     rax, PARAM0
.loop:          mov     [PARAM1], r8w
                mov     [PARAM1 + 2], r9w
                sub     PARAM0, 4
                ja      .loop
                ret

run_loop_store2x4:
                mov     rax, PARAM0
.loop:          mov     [PARAM1], r8d
                mov     [PARAM1 + 4], r9d
                sub     PARAM0, 8
                ja      .loop
                ret

run_loop_store2x8:
                mov     rax, PARAM0
.loop:          mov     [PARAM1], r8
                mov     [PARAM1 + 8], r9
                sub     PARAM0, 16
                ja      .loop
                ret

run_loop_store2x16:
                mov     rax, PARAM0
.loop:          vmovups [PARAM1], xmm0
                vmovups [PARAM1 + 16], xmm1
                sub     PARAM0, 32
                ja      .loop
                ret

run_loop_store2x32:
                mov     rax, PARAM0
.loop:          vmovups [PARAM1], ymm0
                vmovups [PARAM1 + 32], ymm1
                sub     PARAM0, 64
                ja      .loop
                ret

run_loop_store2x64:
                mov     rax, PARAM0
.loop:          vmovups [PARAM1], zmm0
                vmovups [PARAM1 + 64], zmm1
                sub     PARAM0, 128
                ja      .loop
                ret

