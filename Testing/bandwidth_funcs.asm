global run_write_loop
global run_empty_loop
global run_nop_loop
; @TODO: long nop

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%else
%define PARAM0 rdi
%define PARAM1 rsi
%endif

section .text

; proc run_write_loop: writes to given memory
; output : processed bytes count -> rax
run_write_loop: xor     rax, rax 
.loop:          mov     [PARAM1 + rax], al
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

; proc run_empty_loop: runs loop with no body
; output : processed bytes count -> rax
run_empty_loop: xor     rax, rax 
.loop:          inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

; proc run_nop_loop: runs loops with nop as body
; output : processed bytes count -> rax
run_nop_loop:   xor     rax, rax 
.loop:          nop
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

