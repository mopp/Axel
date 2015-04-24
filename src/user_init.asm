; vim:ft=nasm:foldmethod=marker
bits 32

org 0x1000

; init must be here because this program is executed head to tail
init:
    ; Call execve
    push 0
    push 0
    push user_main_path
    mov eax, 0x0b
    int 0x80

    add esp, 12
.loop
    jmp .loop
    ; FIXME for exit

user_main_path:
    db "/boot/user_main", 0
