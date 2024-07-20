;; ..\..\computer_enhance\perfaware\part1\listing_0063_QuadScalarPtr disassembly ;;
bits 16
mov di, 8
mov bp, 1000
mov byte [bp], 9
mov byte [bp+1], 17
mov byte [bp+2], 23
mov byte [bp+3], 4
mov byte [bp+4], 27
mov byte [bp+5], 41
mov byte [bp+6], 39
mov byte [bp+7], 31
xor ax, ax
cmp di, 4
jb $+34+2
shr di, 1
shr di, 1
xor ax, ax
xor bx, bx
xor dx, dx
add al, [bp]
add bl, [bp+1]
add bh, [bp+2]
add dl, [bp+3]
add bp, 4
dec di
jne $-18+2
add al, bl
add bh, dl
add al, bh
ret
