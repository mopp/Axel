; vim:ft=nasm:foldmethod=marker
bits 32

org 0x1000


main:
    xor eax, 0xEFEF
    jmp main

    call fork

    cmp eax, 0
    jnz .loop_parent

.loop_child:
    xor eax, 0xBDBD
    mov word [0x1000], 0xBDBD
    jmp .loop_child

.loop_parent:
    xor eax, 0xACAC
    mov word [0x1000], 0xACAC
    jmp .loop_parent


execve:
    ; execve
    push 0
    push 0
    push path
    mov eax, 0x0b
    int 0x80

    add esp, 12
    ret


fork:
    mov eax, 0x02
    int 0x80
    ret


path db "main"
