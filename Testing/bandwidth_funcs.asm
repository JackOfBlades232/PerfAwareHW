global run_write_loop
global run_empty_loop
global run_nop_loop
global run_nop3_loop
global run_3nop1_loop
global run_nop9_loop
global run_3nop3_loop
global run_9nop_loop

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%else
%define PARAM0 rdi
%define PARAM1 rsi
%endif

%define NOP2 db 0x66, 0x90
%define NOP3 db 0x0F, 0x1F, 0x00
%define NOP4 db 0x0F, 0x1F, 0x40, 0x00
%define NOP5 db 0x0F, 0x1F, 0x44, 0x00, 0x00
%define NOP6 db 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00
%define NOP7 db 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00
%define NOP8 db 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00
%define NOP9 db 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00

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

; proc run_nop3_loop: runs loops with a 3-byte nop as body
; output : processed bytes count -> rax
run_nop3_loop:  xor     rax, rax 
.loop:          NOP3
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

; proc run_3nop1_loop: runs loops with a 3 1-byte nops as body
; output : processed bytes count -> rax
run_3nop1_loop: xor     rax, rax 
.loop:          nop
                nop
                nop
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret


; proc run_nop9_loop: runs loops with a 9-byte nop as body
; output : processed bytes count -> rax
run_nop9_loop:  xor     rax, rax 
.loop:          NOP9
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

; proc run_3nop3_loop: runs loops with a 3 3-byte nops as body
; output : processed bytes count -> rax
run_3nop3_loop: xor     rax, rax 
.loop:          NOP3
                NOP3
                NOP3
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

; proc run_9nop_loop: runs loops with a 9 1-byte nops as body
; output : processed bytes count -> rax
run_9nop_loop:  xor     rax, rax 
.loop:          nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                nop
                inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret
