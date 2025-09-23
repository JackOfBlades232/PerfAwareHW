global run_loop_align64
global run_loop_align1
global run_loop_align3
global run_loop_align15
global run_loop_align31
global run_loop_align63
global run_loop_align127
global run_loop_align255
global run_loop_align4095

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%else
%define PARAM0 rdi
%define PARAM1 rsi
%endif

section .text

run_loop_align64:
align 256
                xor     rax, rax 
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align1:
                xor     rax, rax 
align 256
                nop
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align3:
                xor     rax, rax 
align 256
%rep 3
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align15:
                xor     rax, rax 
align 256
%rep 15
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align31:
                xor     rax, rax 
align 256
%rep 31
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align63:
                xor     rax, rax 
align 256
%rep 63
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align127:
                xor     rax, rax 
align 256
%rep 127
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align255:
                xor     rax, rax 
align 256
%rep 255
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

run_loop_align4095:
                xor     rax, rax 
align 4096
%rep 4095
                nop
%endrep
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

