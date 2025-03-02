; all is 2x cause I know i have 2 load and 2 store ports
global run_loop_load
global run_loop_store

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%define PARAM1 r8
%else
%define PARAM0 rdi
%define PARAM1 rsi
%define PARAM2 rdx
%endif

section .text

run_loop_load:
                xor     rax, rax
.loop:          mov     r9, rax
                and     r9, PARAM2
                vmovups zmm0, [PARAM1 + r9]
                vmovups zmm1, [PARAM1 + r9 + 64]
                add     rax, 128
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_store:
                xor     rax, rax
.loop:          mov     r9, rax
                and     r9, PARAM2
                vmovups [PARAM1 + r9], zmm0
                vmovups [PARAM1 + r9 + 64], zmm1
                add     rax, 128
                cmp     rax, PARAM0
                jb      .loop
                ret
