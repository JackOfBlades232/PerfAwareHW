;; test_sim_input disassembly ;;
bits 16
mov ax, 4096
mov ss, ax
add ax, 4096
mov ds, ax
add ax, 4096
mov es, ax
mov sp, -2
mov bp, sp
push ds
xor ax, ax
mov ds, ax
mov word [0], 9
mov word [2], 5
mov word [4], 0
mov word [6], 6
mov word [8], 3
mov word [10], 6
mov word [12], 6
mov word [14], 6
mov word [16], 13
mov word [18], 6
pop ds
jmp $+26+2
mov ax, 666
out 0, ax
hlt
iret
in al, 0
iret
out 1, al
iret
mov ax, 3
out 0, ax
wait
iret
mov ax, 4221
out 0, ax
iret
xor bx, bx
mov cx, 2
call $+34+3
call $-61+3
mov word [bp-100], 64
call word [bp-100]
call 4:15
mov word [bp-99], 15
mov word [bp-97], 4
call far [bp-99]
loop $-34+2
jmp $+479+3
mov ax, [bx]
push ax
out 44, ax
add bx, 2
mov word [bx], ax
sub word [bx], 15
pop dx
ret
adc ax, -11111
pushf
mov ah, 252
sahf
sub bx, -2
lahf
popf
retf
xor ax, ax
mov al, 9
mov bl, 7
mov dx, 15
inc dx
add al, bl
aaa
out 0, ax
mov al, 7
mov bl, 9
sub al, bl
aas
out 0, ax
mov al, 121
mov dl, 25
add al, dl
daa
out 0, ax
mov dl, 105
dec dl
mov al, 130
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
jmp $+388+3
mov ax, 123
mov bh, 222
mul bx
mov cx, 323
div cx
mov al, 253
mov ch, 244
imul ch
mov cl, 251
idiv cl
mov al, 8
mov bl, 7
mul bl
aam
mov ax, 1543
mov bl, 8
aad
div bl
out 0, ax
mov ax, dx
out 1, ax
jmp $+340+3
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
mov byte es:[di], 104
mov byte es:[di+1], 101
mov byte es:[di+2], 108
mov byte es:[di+3], 111
mov byte es:[di+4], 0
xor ax, ax
sahf
mov cx, 6
repnz scasb
xor di, di
add al, 1
mov byte [si], 119
mov byte [si+1], 104
mov byte [si+2], 97
mov byte [si+3], 116
mov cx, 4
rep movsb
xor si, si
xor di, di
mov byte [si], 119
mov byte [si+1], 104
mov byte [si+2], 104
mov byte [si+3], 104
mov cx, 4
rep cmpsb
nop
jmp $+203+3
xor ax, ax
mov bx, 255
mov cx, 193
and bx, cx
or cl, bl
test bx, 35
not bx
or cx, 14
and ax, 14
xor dx, 14
or word [bx], 14
and byte [bp+13], 14
xor word [di], 14
mov ax, 3
shl ax, 1
mov cl, 3
shl al, cl
shr al, 1
not al
dec cl
sar al, cl
mov bx, -4866
mov cl, 4
rol bx, 1
rcl bx, cl
mov dl, 131
mov cl, 3
ror dl, cl
stc
rcr dl, 1
jmp $+128+3
int 1
int 2
int3
into
mov ax, 32766
add ax, 15
into
jmp $+113+2
xor ax, ax
xor bx, bx
mov dx, -4095
mov ax, -4097
mov bx, 2
idiv bx
jmp $+96+2
mov ax, 3
mov bx, 8
xchg ax, bx
mov word [bp], 9
xchg bl, [bp]
mov al, 223
cbw
cwd
xor bx, bx
mov byte [bx+36], 97
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
jmp $+28+2
mov dx, 2
dec dx
jne $-3+2
mov bl, 3
dec bl
js $+2+2
jmp $-6+2
mov ax, 3
mov cx, 4
cmp ax, cx
jae $+4+2
out 0, ax
jmp $+0+2
hlt
