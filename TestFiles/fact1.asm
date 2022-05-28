in
pop ax

push 1
pop bx

call :fact
push bx
out
hlt

:fact

push ax
push bx

je :end

push ax
dec
pop ax

call :fact

push ax
inc
pop ax

push ax
push bx
mul

pop bx

:end

ret
