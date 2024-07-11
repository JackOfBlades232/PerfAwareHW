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
jmp test_funcs

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

done:
hlt
