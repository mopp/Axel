; vim:ft=nasm:foldmethod=marker
bits 32

org 0x1000

main:
    ; execve
    push 0
    push 0
    push path
    mov eax, 0x0b
    int 0x80

    add esp, 12
.loop:
    call func

    jmp .loop

func:
    xor eax, 0xACAC
    ret

path db "main"
