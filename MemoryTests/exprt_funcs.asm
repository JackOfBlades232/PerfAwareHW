global run_loop_load
global run_loop_load2
global run_loop_load3
global run_loop_load4
global run_loop_load2xbyte
global run_loop_store
global run_loop_store2
global run_loop_store3
global run_loop_store4
global run_loop_store2xbyte

%ifdef _WIN32
%define PARAM0 rcx
%define PARAM1 rdx
%else
%define PARAM0 rdi
%define PARAM1 rsi
%endif

section .text

run_loop_load:
.loop:          mov     rax, [PARAM1]
                dec     PARAM0
                jnle    .loop
                ret

run_loop_load2:
.loop:          mov     rax, [PARAM1]
                mov     rax, [PARAM1]
                sub     PARAM0, 2
                jnle    .loop
                ret

run_loop_load3:
.loop:          mov     rax, [PARAM1]
                mov     rax, [PARAM1]
                mov     rax, [PARAM1]
                sub     PARAM0, 3
                jnle    .loop
                ret

run_loop_load4:
.loop:          mov     rax, [PARAM1]
                mov     rax, [PARAM1]
                mov     rax, [PARAM1]
                mov     rax, [PARAM1]
                sub     PARAM0, 4
                jnle    .loop
                ret

run_loop_load2xbyte:
.loop:          mov     al, [PARAM1]
                mov     al, [PARAM1]
                sub     PARAM0, 2
                jnle    .loop
                ret

run_loop_store:
                xor     rax, rax
.loop:          mov     [PARAM1], rax
                dec     PARAM0
                jnle    .loop
                ret

run_loop_store2:
                xor     rax, rax
.loop:          mov     [PARAM1], rax
                mov     [PARAM1], rax
                sub     PARAM0, 2
                jnle    .loop
                ret

run_loop_store3:
                xor     rax, rax
.loop:          mov     [PARAM1], rax
                mov     [PARAM1], rax
                mov     [PARAM1], rax
                sub     PARAM0, 3
                jnle    .loop
                ret

run_loop_store4:
                xor     rax, rax
.loop:          mov     [PARAM1], rax
                mov     [PARAM1], rax
                mov     [PARAM1], rax
                mov     [PARAM1], rax
                sub     PARAM0, 4
                jnle    .loop
                ret

run_loop_store2xbyte:
                xor     rax, rax
.loop:          mov     [PARAM1], al
                mov     [PARAM1], al
                sub     PARAM0, 2
                jnle    .loop
                ret
