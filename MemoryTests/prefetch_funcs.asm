global ll_foreach
global ll_foreach_prefetch

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

ll_foreach:
                xor         rax, rax
.loop:          mov         r10, [PARAM0]
                vmovups     zmm0, [PARAM0]
                mov         r11, PARAM2
.work_loop:     vmovups     zmm1, zmm0
                vaddps      zmm0, zmm1
                sub         r11, 1
                ja          .work_loop
                mov         PARAM0, r10
                add         rax, 64
                cmp         rax, PARAM1
                jb          .loop
                ret

ll_foreach_prefetch:
                xor         rax, rax
.loop:          mov         r10, [PARAM0]
                vmovups     zmm0, [PARAM0]
                prefetcht0  [r10]
                mov         r11, PARAM2
.work_loop:     vmovups     zmm1, zmm0
                vaddps      zmm0, zmm1
                sub         r11, 1
                ja          .work_loop
                mov         PARAM0, r10
                add         rax, 64
                cmp         rax, PARAM1
                jb          .loop
                ret
