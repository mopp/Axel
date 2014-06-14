;---------------------------------------------------------------------
; vim:ft=nasm:foldmethod=marker
; File: src/interrupt_handler.asm
; Description: It provides interrupt handler entry point
;---------------------------------------------------------------------
    bits 32

%macro push_all 0
        push ds
        push es
        push fs
        push gs
        pushad
        pushfd
%endmacro


%macro pop_all 0
        popfd
        popad
        pop gs
        pop fs
        pop es
        pop ds
%endmacro


; macro to make interrupt_handler.
%macro make_interrupt_handler 1
    global asm_%1
    extern %1

asm_%1:
    push_all

    ; set stack pointer as function argument.
    mov eax, esp

    call %1

    pop_all

    iretd
%endmacro


section .text

; making interrupt handler.
; first argument is handler function in C.
make_interrupt_handler interrupt_keybord
make_interrupt_handler interrupt_mouse
make_interrupt_handler interrupt_timer

