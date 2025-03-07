; all is 2x cause I know i have 2 load and 2 store ports
global run_loop_load_pot
global run_loop_store_pot
global run_loop_load_npot
global run_loop_store_npot

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

run_loop_load_pot:
                xor     rax, rax
.loop:          mov     r9, rax
                and     r9, PARAM2
                vmovups zmm0, [PARAM1 + r9]
                vmovups zmm0, [PARAM1 + r9 + 64]
                vmovups zmm0, [PARAM1 + r9 + 128]
                vmovups zmm0, [PARAM1 + r9 + 192]
                vmovups zmm0, [PARAM1 + r9 + 256]
                vmovups zmm0, [PARAM1 + r9 + 320]
                vmovups zmm0, [PARAM1 + r9 + 384]
                vmovups zmm0, [PARAM1 + r9 + 448]
                add     rax, 512
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_store_pot:
                xor     rax, rax
.loop:          mov     r9, rax
                and     r9, PARAM2
                vmovups [PARAM1 + r9], zmm0
                vmovups [PARAM1 + r9 + 64], zmm0
                vmovups [PARAM1 + r9 + 128], zmm0
                vmovups [PARAM1 + r9 + 192], zmm0
                vmovups [PARAM1 + r9 + 256], zmm0
                vmovups [PARAM1 + r9 + 320], zmm0
                vmovups [PARAM1 + r9 + 384], zmm0
                vmovups [PARAM1 + r9 + 448], zmm0
                add     rax, 512
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_load_npot:
                xor     rax, rax
.loop:          mov     r9, PARAM2
.inner_loop:    vmovups zmm0, [PARAM1 + r9]
                vmovups zmm0, [PARAM1 + r9 + 64]
                vmovups zmm0, [PARAM1 + r9 + 128]
                vmovups zmm0, [PARAM1 + r9 + 192]
                vmovups zmm0, [PARAM1 + r9 + 256]
                vmovups zmm0, [PARAM1 + r9 + 320]
                vmovups zmm0, [PARAM1 + r9 + 384]
                vmovups zmm0, [PARAM1 + r9 + 448]
                sub     r9, 512
                ja      .inner_loop
                add     rax, PARAM2
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_store_npot:
                xor     rax, rax
.loop:          mov     r9, PARAM2
.inner_loop:    vmovups [PARAM1 + r9], zmm0
                vmovups [PARAM1 + r9 + 64], zmm0
                vmovups [PARAM1 + r9 + 128], zmm0
                vmovups [PARAM1 + r9 + 192], zmm0
                vmovups [PARAM1 + r9 + 256], zmm0
                vmovups [PARAM1 + r9 + 320], zmm0
                vmovups [PARAM1 + r9 + 384], zmm0
                vmovups [PARAM1 + r9 + 448], zmm0
                sub     r9, 512
                ja      .inner_loop
                add     rax, PARAM2
                cmp     rax, PARAM0
                jb      .loop
                ret
