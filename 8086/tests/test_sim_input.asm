bits 16

; @TODO: Verify clocks

; Prep segments
; cs is 0 already
mov ax, 0x1000
mov ss, ax
add ax, 0x1000
mov ds, ax
add ax, 0x1000
mov es, ax

; Prep stack
mov sp, 0xFFFF
mov bp, sp

; set label here
jmp test_funcs
; jmp test_arithm
; jmp test_muldiv

; @TEST: function call
; it tests:
;       unconditional call & ret
;       unconditional intrasegment jmp
; also:
;       looping with cx
;       stack operations
;       out for logging
;       mov/add/sub/xor, to limited extent
;       lahf/sahf/pushf/popf
;
; Correct result:
; Registers state:
;      ax: 0xffc4 (65476)
;      bx: 0x000a (10)
;      cx: 0x0000 (0)
;      dx: 0xffc4 (65476)
;      sp: 0xffff (65535)
;      bp: 0xffff (65535)
;      si: 0x0000 (0)
;      di: 0x0000 (0)
;      es: 0x3000 (12288)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x0032 (50)
;   flags: AS
;
; Total clocks: 644
test_funcs:

xor bx, bx
mov cx, 2
lp:
call func
call 0x40
mov word [bp-100], 0x40
call [bp-100]

call 4:15 ; ffunc
mov word [bp-99], 15
mov word [bp-97], 4
call far [bp-99]

loop lp

jmp done

func:
mov ax, [bx]
push ax
out 44, ax
add bx, 2
mov [bx], ax
sub word [bx], 0xF
pop dx
ret

ffunc:
adc ax, -11111
pushf
mov ah, 0xFC
sahf
sub bx, 0xFFFE
lahf
popf
retf

; @TEST: addition/subtraction arithmetics
; it tests:
;   add, adc, aaa, daa
;   sub, sbb, aas, das
;   inc, dec
;   cmp, neg
; also:
;   xchg
;
; Correct result:
; Registers state:
;      ax: 0x0015 (21)
;      bx: 0x0009 (9)
;      cx: 0xffff (65535)
;      dx: 0xff98 (65432)
;      sp: 0xffff (65535)
;      bp: 0xffff (65535)
;      si: 0x0000 (0)
;      di: 0x0000 (0)
;      es: 0x3000 (12288)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x0070 (112)
;   flags: CS
;
; Total clocks: 204
test_arithm:

xor ax, ax
mov al, 9
mov bl, 7
mov dx, 0xF
inc dx
add al, bl
aaa
out 0, ax

mov al, 7
mov bl, 9
sub al, bl
aas
out 0, ax

mov al, 0x79
mov dl, 0x19
add al, dl
daa
out 0, ax

mov dl, 0x69
dec dl
mov al, 0x82
sub al, dl
das
out 0, ax

adc ax, dx
sub cx, 1
sbb ax, dx
out 0, ax

cmp ax, 1234
neg dx
out 0, ax

jmp done

; @TEST: mul/div arithmetics
; it tests:
;   mul, imul, div, idiv
;   aam, aad
;
; Correct result:
; Registers state:
;      ax: 0x0125 (293)
;      bx: 0xde08 (56840)
;      cx: 0xf4fb (62715)
;      dx: 0x0125 (293)
;      sp: 0xffff (65535)
;      bp: 0xffff (65535)
;      si: 0x0000 (0)
;      di: 0x0000 (0)
;      es: 0x3000 (12288)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x009f (159)
;   flags: CO
; 
; Total clocks: 931
test_muldiv:

mov ax, 123
mov bh, 222
mul bx

mov cx, 323
div cx

mov al, -3
mov ch, -12
imul ch

mov cl, -5
idiv cl

mov al, 0x8
mov bl, 0x7
mul bl
aam

mov ax, 0x607
mov bl, 8
aad
div bl

out 0, ax
mov ax, dx
out 1, ax

jmp done

done:
hlt
