bits 16

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
; jmp test_funcs
jmp test_arithm

; @TEST: function call
; it tests:
;       unconditional intrasegment call & ret
;       unconditional intrasegment jmp
; also:
;       looping with cx
;       stack operations
;       out for logging
;       mov/add/sub/xor, to limited extent
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
; Total clocks: 644 // @TODO: this is not sure info
test_funcs:

xor bx, bx
mov cx, 5
lp:
call func
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

; @TEST: addition/subtraction arithmetics
; it tests:
;   add, adc, aaa, daa
;   sub, sbb, aas, das
;   inc, dec
;   cmp, neg
;
; Correct result:
; Registers state:
;
; Total clocks:
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

done:
hlt
