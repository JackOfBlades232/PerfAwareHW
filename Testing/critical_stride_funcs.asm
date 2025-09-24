global run_loop_load_npot_strided

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

run_loop_load_npot_strided:
                xor     rax, rax
                push    r12
                push    r13
                push    r14
                mov     r12, PARAM3
                shl     r12, 1
                mov     r13, PARAM3
                add     r13, r12
                mov     r14, r12
                shl     r14, 1
.loop:          mov     r10, PARAM2
                mov     r11, PARAM1
.inner_loop:    vmovups zmm0, [r11]
                vmovups zmm0, [r11 + PARAM3]
                vmovups zmm0, [r11 + r12]
                vmovups zmm0, [r11 + r13]
                add     r11, r14
                sub     r10, 4
                ja      .inner_loop
                add     rax, PARAM2
                cmp     rax, PARAM0
                jb      .loop
                pop     r14
                pop     r13
                pop     r12
                ret
