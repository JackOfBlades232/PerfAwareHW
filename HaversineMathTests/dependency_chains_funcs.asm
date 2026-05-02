global fma_depchain_asm
global fma_depchain_interleaved2_asm
global fma_depchain_interleaved4_asm
global fma_depchain_interleaved8_asm
global fma_depchain_interleaved8x2_asm
global fma_depchain_block8_asm
global fma_depchain_block16_asm

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

%macro CONST4X 1
dq %1, %1, %1, %1
%endmacro

section .text

mul_a:          CONST4X 1.01
mul_b:          CONST4X 1.001
addend:         CONST4X 0.01

fma_depchain_asm:
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          vmovdqu     xmm2, [rel mul_a]
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
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          vmovdqu     xmm2, [rel mul_a]
                vmovdqu     xmm3, xmm2
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
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          vmovdqu     xmm2, [rel mul_a]
                vmovdqu     xmm3, xmm2
                vmovdqu     xmm4, xmm2
                vmovdqu     xmm5, xmm2
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
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          vmovdqu     xmm2, [rel mul_a]
                vmovdqu     xmm3, xmm2
                vmovdqu     xmm4, xmm2
                vmovdqu     xmm5, xmm2
                vmovdqu     xmm6, xmm2
                vmovdqu     xmm7, xmm2
                vmovdqu     xmm8, xmm2
                vmovdqu     xmm9, xmm2
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
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          vmovdqu     xmm2, [rel mul_a]
                vmovdqu     xmm3, xmm2
                vmovdqu     xmm4, xmm2
                vmovdqu     xmm5, xmm2
                vmovdqu     xmm6, xmm2
                vmovdqu     xmm7, xmm2
                vmovdqu     xmm8, xmm2
                vmovdqu     xmm9, xmm2
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

fma_depchain_block8_asm:
                lea         rax, [rsp - 8*0x10]
                and         rax, ~0xF
                mov         r11, rax
                sub         r11, 8*0x10
                vmovdqu     xmm2, [rel mul_a]
                mov         r10, 8
.clear_loop:    vmovdqu     [rax], xmm2
                add         rax, 0x10
                dec         r10
                jg          .clear_loop
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          mov         rax, r11
                add         rax, 8*0x10
                mov         r10, PARAM1
.inner_loop:    mov         r9, 8
.block_loop:    vmovdqu     xmm2, [rax]
                add         rax, 0x10
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vmovdqu     [r11], xmm2
                add         r11, 0x10
                dec         r9
                jg          .block_loop
                sub         r11, 8*0x10
                mov         rax, r11
                sub         r10, 8
                jg          .inner_loop
                sub         PARAM0, 8
                jg          .loop
                ret

fma_depchain_block16_asm:
                lea         rax, [rsp - 16*0x10]
                and         rax, ~0xF
                mov         r11, rax
                sub         r11, 16*0x10
                vmovdqu     xmm2, [rel mul_a]
                mov         r10, 16
.clear_loop:    vmovdqu     [rax], xmm2
                add         rax, 0x10
                dec         r10
                jg          .clear_loop
                vmovdqu     xmm0, [rel mul_b]
                vmovdqu     xmm1, [rel addend]
.loop:          mov         rax, r11
                add         rax, 16*0x10
                mov         r10, PARAM1
.inner_loop:    mov         r9, 16
.block_loop:    vmovdqu     xmm2, [rax]
                add         rax, 0x10
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vfmadd213sd xmm2, xmm0, xmm1
                vmovdqu     [r11], xmm2
                add         r11, 0x10
                dec         r9
                jg          .block_loop
                sub         r11, 16*0x10
                mov         rax, r11
                sub         r10, 8
                jg          .inner_loop
                sub         PARAM0, 16
                jg          .loop
                ret
