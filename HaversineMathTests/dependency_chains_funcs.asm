global fma_depchain_asm
global fma_depchain_interleaved2_asm
global fma_depchain_interleaved4_asm
global fma_depchain_interleaved8_asm
global fma_depchain_interleaved8x2_asm

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

fma_depchain_asm:
.loop:          vpxor       xmm0, xmm0
                vpxor       xmm1, xmm1
                vpxor       xmm2, xmm2
                mov         r10, PARAM1
.inner_loop:    vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                sub         r10, 8
                jg          .inner_loop
                dec         PARAM0
                jg          .loop
                ret

fma_depchain_interleaved2_asm:
.loop:          vpxor       xmm0, xmm0
                vpxor       xmm1, xmm1
                vpxor       xmm2, xmm2
                vpxor       xmm3, xmm3
                mov         r10, PARAM1
.inner_loop:    vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                sub         r10, 4
                jg          .inner_loop
                sub         PARAM0, 2
                jg          .loop
                ret

fma_depchain_interleaved4_asm:
.loop:          vpxor       xmm0, xmm0
                vpxor       xmm1, xmm1
                vpxor       xmm2, xmm2
                vpxor       xmm3, xmm3
                vpxor       xmm4, xmm4
                vpxor       xmm5, xmm5
                mov         r10, PARAM1
.inner_loop:    vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm4, xmm0, xmm1
                vfmadd213sd xmm5, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm4, xmm0, xmm1
                vfmadd213sd xmm5, xmm0, xmm1
                sub         r10, 2
                jg          .inner_loop
                sub         PARAM0, 4
                jg          .loop
                ret

fma_depchain_interleaved8_asm:
                vmovdqu     [rsp - 0x10], xmm6
                vmovdqu     [rsp - 0x20], xmm7
                vmovdqu     [rsp - 0x30], xmm8
                vmovdqu     [rsp - 0x40], xmm9
.loop:          vpxor       xmm0, xmm0
                vpxor       xmm1, xmm1
                vpxor       xmm2, xmm2
                vpxor       xmm3, xmm3
                vpxor       xmm4, xmm4
                vpxor       xmm5, xmm5
                vpxor       xmm6, xmm6
                vpxor       xmm7, xmm7
                vpxor       xmm8, xmm8
                vpxor       xmm9, xmm9
                mov         r10, PARAM1
.inner_loop:    vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm4, xmm0, xmm1
                vfmadd213sd xmm5, xmm0, xmm1
                vfmadd213sd xmm6, xmm0, xmm1
                vfmadd213sd xmm7, xmm0, xmm1
                vfmadd213sd xmm8, xmm0, xmm1
                vfmadd213sd xmm9, xmm0, xmm1
                sub         r10, 1
                jg          .inner_loop
                sub         PARAM0, 8
                jg          .loop
                vmovdqu     xmm6, [rsp - 0x10]
                vmovdqu     xmm7, [rsp - 0x20]
                vmovdqu     xmm8, [rsp - 0x30]
                vmovdqu     xmm9, [rsp - 0x40]
                ret

fma_depchain_interleaved8x2_asm:
                vmovdqu     [rsp - 0x10], xmm6
                vmovdqu     [rsp - 0x20], xmm7
                vmovdqu     [rsp - 0x30], xmm8
                vmovdqu     [rsp - 0x40], xmm9
.loop:          vpxor       xmm0, xmm0
                vpxor       xmm1, xmm1
                vpxor       xmm2, xmm2
                vpxor       xmm3, xmm3
                vpxor       xmm4, xmm4
                vpxor       xmm5, xmm5
                vpxor       xmm6, xmm6
                vpxor       xmm7, xmm7
                vpxor       xmm8, xmm8
                vpxor       xmm9, xmm9
                mov         r10, PARAM1
.inner_loop:    vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm4, xmm0, xmm1
                vfmadd213sd xmm5, xmm0, xmm1
                vfmadd213sd xmm6, xmm0, xmm1
                vfmadd213sd xmm7, xmm0, xmm1
                vfmadd213sd xmm8, xmm0, xmm1
                vfmadd213sd xmm9, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm3, xmm0, xmm1
                vfmadd213sd xmm4, xmm0, xmm1
                vfmadd213sd xmm5, xmm0, xmm1
                vfmadd213sd xmm6, xmm0, xmm1
                vfmadd213sd xmm7, xmm0, xmm1
                vfmadd213sd xmm8, xmm0, xmm1
                vfmadd213sd xmm9, xmm0, xmm1
                sub         r10, 2
                jg          .inner_loop
                sub         PARAM0, 8
                jg          .loop
                vmovdqu     xmm6, [rsp - 0x10]
                vmovdqu     xmm7, [rsp - 0x20]
                vmovdqu     xmm8, [rsp - 0x30]
                vmovdqu     xmm9, [rsp - 0x40]
                ret
