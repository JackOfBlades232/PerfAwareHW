bits 16

; @TODO: Verify clocks

; @NOTE: these are tests for stuff not
; covered by listings from computer_enhance

; Prep segments
; cs is 0 already
mov ax, 0x1000
mov ss, ax
add ax, 0x1000
mov ds, ax
add ax, 0x1000
mov es, ax

; Prep stack
mov sp, 0xFFFE
mov bp, sp

; Prep interrupt pointer table (overwrites segment init code)
push ds
xor ax, ax
mov ds, ax
mov word [0],  0x9
mov word [2],  0x5
mov word [4],  0x0
mov word [6],  0x6
mov word [8],  0x3
mov word [10], 0x6
mov word [12], 0x6
mov word [14], 0x6
mov word [16], 0xd
mov word [18], 0x6
pop ds

; set label here
; jmp test_funcs
; jmp test_arithm
; jmp test_muldiv
; jmp test_string
; jmp test_logic
; jmp test_interrupts
; jmp test_exceptions
jmp test_misc

; "interrupt table"
i_zero_div:
mov ax, 666
out 0, ax
hlt
iret

i_in:
in al, 0
iret

i_out:
out 1, al
iret

i_three:
mov ax, 3
out 0, ax
wait
iret

i_over:
mov ax, 4221
out 0, ax
iret

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
;      ax: 0x1515 (5397)
;      bx: 0x0014 (20)
;      cx: 0x0000 (0)
;      dx: 0xffe2 (65506)
;      sp: 0xffff (65535)
;      bp: 0xffff (65535)
;      si: 0x0000 (0)
;      di: 0x0000 (0)
;      es: 0x3000 (12288)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x00f3 (243)
;   flags: AS
; 
; Total clocks: 1301
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

; @TEST: string instructions
; it tests:
;   scas/lods/cmps/stos/movs + rep prefix
; also:
;   clc/cmc/cld/std/cli/sti
;   seg prefixes
;   nop
;
; Correct result:
; Registers state:
;      ax: 0x0001 (1)
;      bx: 0x0000 (0)
;      cx: 0x0001 (1)
;      dx: 0x0000 (0)
;      sp: 0xffff (65535)
;      bp: 0xffff (65535)
;      si: 0x0003 (3)
;      di: 0x0003 (3)
;      es: 0x3000 (12288)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x0155 (341)
;   flags:
; 
; Total clocks: 561
test_string:

xor ax, ax
sub ax, 16
clc
cmc
cmc
stc

std
cld
sti
cli

mov cx, 5
mov di, 0
mov al, 55


mov word [si], 22
mov byte [si+2], 11

stosb
lodsw
movsb

xor di, di
scasw

xor si, si
xor di, di
mov word [si], 1222
mov word es:[di], 1
cmpsw

xor si, si
xor di, di
mov byte es:[di],   'h'
mov byte es:[di+1], 'e'
mov byte es:[di+2], 'l'
mov byte es:[di+3], 'o'
mov byte es:[di+4],  0

xor ax, ax
sahf

mov cx, 6
repnz scasb

xor di, di
add al, 1
mov byte [si],   'w'
mov byte [si+1], 'h'
mov byte [si+2], 'a'
mov byte [si+3], 't'

mov cx, 4
rep movsb

xor si, si
xor di, di
mov byte [si],   'w'
mov byte [si+1], 'h'
mov byte [si+2], 'h'
mov byte [si+3], 'h'

mov cx, 4
repz cmpsb

nop

jmp done

; @TEST: logic and shifts
; it tests:
;   scas/lods/cmps/stos/movs + rep prefix
; also:
;
; Correct result:
; Registers state:
;      ax: 0x00f9 (249)
;      bx: 0x9fde (40926)
;      cx: 0x0003 (3)
;      dx: 0x00b8 (184)
;      sp: 0xffff (65535)
;      bp: 0xffff (65535)
;      si: 0x0000 (0)
;      di: 0x0000 (0)
;      es: 0x3000 (12288)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x018a (394)
;   flags: CPSO
; 
; Total clocks: 196
test_logic:

xor ax, ax
mov bx, 0xFF
mov cx, 0xC1
and bx, cx
test bx, 0x23
not bx

; @TODO: nasm seems to be smoking trees when it comes to some variations.
;        Check it out.
; or cx, 0xE
; and ax, 0xE

mov ax, 3

shl ax, 1
mov cl, 3
shl al, cl

shr al, 1
not al
dec cl
sar al, cl

mov bx, 0xECFE
mov cl, 4
rol bx, 1
rcl bx, cl

mov dl, 0x83
mov cl, 3
ror dl, cl

stc
rcr dl, 1

jmp done

; @TEST: interrupts
; it tests:
;   int/int3/into/iret
;
; Correct result:
; Registers state:
;      ax: 0x029a (666)
;      bx: 0x0000 (0)
;      cx: 0x0000 (0)
;      dx: 0x0000 (0)
;      sp: 0xfff9 (65529)
;      bp: 0xffff (65535)
;      si: 0x0000 (0)
;      di: 0x0000 (0)
;      es: 0x3000 (12288)
;      cs: 0x0005 (5)
;      ss: 0x1000 (4096)
;      ds: 0x2000 (8192)
;      ip: 0x000f (15)
;   flags: ASO
; 
; Total clocks: 564
test_interrupts:

int 1
int 2
int3
into
mov ax, 0x7FFE
add ax, 0xF
into

jmp done

; @TEST: exceptions
; it tests:
;   div/idiv by 0 or too large quotient
;
; Correct result: not really relevant, just check for exception 0 thrown and handled
test_exceptions:

xor ax, ax
xor bx, bx
; div bx

mov dx, -0xFFF
mov ax, 0xEFFF
mov bx, 2
idiv bx

jmp done

; @TEST: misc
; it tests:
;   xchg lea/lds/les cbw/cwd xlat
;
; Correct result:
; Registers state:
;      ax: 0x08a5 (2213)
;      bx: 0x0000 (0)
;      cx: 0x0000 (0)
;      dx: 0x0026 (38)
;      sp: 0xfffe (65534)
;      bp: 0xfffe (65534)
;      si: 0x014d (333)
;      di: 0x000c (12)
;      es: 0x0002 (2)
;      cs: 0x0000 (0)
;      ss: 0x1000 (4096)
;      ds: 0x04c5 (1221)
;      ip: 0x024b (587)
;   flags: PZ
; 
; Total clocks: 495
test_misc:

mov ax, 3
mov bx, 8
xchg ax, bx
mov word [bp], 9
xchg [bp], bl

mov al, -33
cbw
cwd

xor bx, bx
mov byte [bx+36], 'a'
mov al, 36
xlat

mov di, 12
mov si, 333

lea dx, [bp+di+12]

mov word [bx+si-7], 38
mov word [bx+si-5], 1221
mov word [bx+di+1420], 2213
mov word [bx+di+1422], 2

les ax, [bx+di+1420]
lds dx, [bx+si-7]

jmp done

done:
hlt
