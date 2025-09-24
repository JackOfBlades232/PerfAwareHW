; all is 2x cause I know i have 2 load and 2 store ports
global run_loop_load_pot
global run_loop_store_pot
global run_loop_load_npot
global run_loop_store_npot

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%define PARAM2 r8
%define PARAM3 r9
%else
%define PARAM0 rdi
%define PARAM1 rsi
%define PARAM2 rdx
%define PARAM3 rcx
%endif

section .text

run_loop_load_pot:
                xor     rax, rax
.loop:          mov     r10, rax
                and     r10, PARAM2
                lea     r11, [PARAM1 + r10]
                vmovups zmm0, [r11]
                vmovups zmm0, [r11 + 64]
                vmovups zmm0, [r11 + 128]
                vmovups zmm0, [r11 + 192]
                vmovups zmm0, [r11 + 256]
                vmovups zmm0, [r11 + 320]
                vmovups zmm0, [r11 + 384]
                vmovups zmm0, [r11 + 448]
                add     rax, 512
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_store_pot:
                xor     rax, rax
.loop:          mov     r10, rax
                and     r10, PARAM2
                lea     r11, [PARAM1 + r10]
                vmovups [r11], zmm0
                vmovups [r11 + 64], zmm0
                vmovups [r11 + 128], zmm0
                vmovups [r11 + 192], zmm0
                vmovups [r11 + 256], zmm0
                vmovups [r11 + 320], zmm0
                vmovups [r11 + 384], zmm0
                vmovups [r11 + 448], zmm0
                add     rax, 512
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_load_npot:
                xor     rax, rax
.loop:          mov     r10, PARAM2
                mov     r11, PARAM1
.inner_loop:    vmovups zmm0, [r11]
                vmovups zmm0, [r11 + 64]
                vmovups zmm0, [r11 + 128]
                vmovups zmm0, [r11 + 192]
                vmovups zmm0, [r11 + 256]
                vmovups zmm0, [r11 + 320]
                vmovups zmm0, [r11 + 384]
                vmovups zmm0, [r11 + 448]
                add     r11, 512
                sub     r10, 512
                ja      .inner_loop
                add     rax, PARAM2
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_store_npot:
                xor     rax, rax
.loop:          mov     r10, PARAM2
                mov     r11, PARAM1
.inner_loop:    vmovups [r11], zmm0
                vmovups [r11 + 64], zmm0
                vmovups [r11 + 128], zmm0
                vmovups [r11 + 192], zmm0
                vmovups [r11 + 256], zmm0
                vmovups [r11 + 320], zmm0
                vmovups [r11 + 384], zmm0
                vmovups [r11 + 448], zmm0
                add     r11, 512
                sub     r10, 512
                ja      .inner_loop
                add     rax, PARAM2
                cmp     rax, PARAM0
                jb      .loop
                ret
