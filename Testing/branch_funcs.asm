global run_cond_loop

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%else
%define PARAM0 rdi
%define PARAM1 rsi
%endif

; proc run_cond_loop: reads memory, taking a jump if it was odd.
; output : processed bytes count -> rax
run_cond_loop:  xor     rax, rax 
.loop:          mov     r10, [PARAM1 + rax]
                test    r10, 1
                jnz     .skip
                nop
.skip           inc     rax 
                cmp     rax, PARAM0
                jb      .loop
                ret

