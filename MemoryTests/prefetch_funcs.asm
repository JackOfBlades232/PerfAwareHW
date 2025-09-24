global calc_on_data
global calc_on_data_prefetch1
global calc_on_data_prefetch2
global calc_on_data_prefetch4

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

calc_on_data:
                xor         rax, rax
.loop:          mov         r10, [PARAM1]
                vmovups     zmm0, [r10]
                vmovups     zmm1, zmm0
                vmovups     zmm2, zmm0
                vaddps      zmm0, zmm1
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                add         PARAM0, 64
                add         PARAM1, 8
                add         rax, 1
                cmp         rax, PARAM2
                jb          .loop
                shl         rax, 6
                ret

calc_on_data_prefetch1:
                xor         rax, rax
.loop:          mov         r10, [PARAM1]
                vmovups     zmm0, [r10]
                mov         r10, [PARAM1 + 8]
                prefetcht0  [r10]
                vmovups     zmm1, zmm0
                vmovups     zmm2, zmm0
                vaddps      zmm0, zmm1
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                add         PARAM0, 64
                add         PARAM1, 8
                add         rax, 1
                cmp         rax, PARAM2
                jb          .loop
                shl         rax, 6
                ret

calc_on_data_prefetch2:
                xor         rax, rax
.loop:          mov         r10, [PARAM1]
                vmovups     zmm0, [r10]
                mov         r10, [PARAM1 + 8]
                prefetcht0  [r10]
                mov         r10, [PARAM1 + 16]
                prefetcht0  [r10]
                vmovups     zmm1, zmm0
                vmovups     zmm2, zmm0
                vaddps      zmm0, zmm1
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                add         PARAM0, 64
                add         PARAM1, 8
                add         rax, 1
                cmp         rax, PARAM2
                jb          .loop
                shl         rax, 6
                ret

calc_on_data_prefetch4:
                xor         rax, rax
.loop:          mov         r10, [PARAM1]
                vmovups     zmm0, [r10]
                mov         r10, [PARAM1 + 8]
                prefetcht0  [r10]
                mov         r10, [PARAM1 + 16]
                prefetcht0  [r10]
                mov         r10, [PARAM1 + 24]
                prefetcht0  [r10]
                mov         r10, [PARAM1 + 32]
                prefetcht0  [r10]
                vmovups     zmm1, zmm0
                vmovups     zmm2, zmm0
                vaddps      zmm0, zmm1
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                vdivps      zmm0, zmm1, zmm2
                vdivps      zmm1, zmm2, zmm0
                vdivps      zmm2, zmm1, zmm0
                add         PARAM0, 64
                add         PARAM1, 8
                add         rax, 1
                cmp         rax, PARAM2
                jb          .loop
                shl         rax, 6
                ret
