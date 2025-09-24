global memcpy512
global memcpy512_nt

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

memcpy512:
.loop:          vmovups     zmm0, [PARAM1]
                vmovups     [PARAM0], zmm0
                vmovups     zmm0, [PARAM1 + 64]
                vmovups     [PARAM0 + 64], zmm0
                vmovups     zmm0, [PARAM1 + 128]
                vmovups     [PARAM0 + 128], zmm0
                vmovups     zmm0, [PARAM1 + 192]
                vmovups     [PARAM0 + 192], zmm0
                vmovups     zmm0, [PARAM1 + 256]
                vmovups     [PARAM0 + 256], zmm0
                vmovups     zmm0, [PARAM1 + 320]
                vmovups     [PARAM0 + 320], zmm0
                vmovups     zmm0, [PARAM1 + 384]
                vmovups     [PARAM0 + 384], zmm0
                vmovups     zmm0, [PARAM1 + 448]
                vmovups     [PARAM0 + 448], zmm0
                add         PARAM0, 512
                add         PARAM1, 512
                sub         PARAM2, 512
                ja          .loop
                xor         rax, rax
                ret

memcpy512_nt:
.loop:          vmovups     zmm0, [PARAM1]
                vmovntps    [PARAM0], zmm0
                vmovups     zmm0, [PARAM1 + 64]
                vmovntps    [PARAM0 + 64], zmm0
                vmovups     zmm0, [PARAM1 + 128]
                vmovntps    [PARAM0 + 128], zmm0
                vmovups     zmm0, [PARAM1 + 192]
                vmovntps    [PARAM0 + 192], zmm0
                vmovups     zmm0, [PARAM1 + 256]
                vmovntps    [PARAM0 + 256], zmm0
                vmovups     zmm0, [PARAM1 + 320]
                vmovntps    [PARAM0 + 320], zmm0
                vmovups     zmm0, [PARAM1 + 384]
                vmovntps    [PARAM0 + 384], zmm0
                vmovups     zmm0, [PARAM1 + 448]
                vmovntps    [PARAM0 + 448], zmm0
                add         PARAM0, 512
                add         PARAM1, 512
                sub         PARAM2, 512
                ja          .loop
                xor         rax, rax
                ret
