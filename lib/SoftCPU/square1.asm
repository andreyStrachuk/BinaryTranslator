push 1
pop ax
push 1
pop bx
push -6
pop cx

push 0
push ax
je :linear

push cx
push ax
push 4
mul
mul

push bx
push bx
mul

sub

pop dx

push dx
push 0
jb : noroot

push dx

sqrt

pop dx

push dx
push 0
je :oneroot

push 2
push ax
mul

push dx

push -1
push bx
mul

add

div


push 2
push ax
mul

push -1
push bx
mul

push -1
push dx
mul

add

div

hlt


:linear

push 0
push bx

je :free

push bx

push cx
push -1
mul

div

hlt


:free

push 0
push cx

je :inf
push 0


hlt


:inf
push -1


hlt

:oneroot
push ax
push 2
mul


push bx
push -1

div


hlt

:noroot

hlt


