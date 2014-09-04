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
%endmacro


%macro pop_all 0
        popad
        pop gs
        pop fs
        pop es
        pop ds
%endmacro


; macro to make interrupt_handler.
%macro make_interrupt_handler 2
    global asm_%2
    extern %2

asm_%2:
    %if %1 == 0
        ; Interruption has no error_code.
        ; So, push dummy error code.
        push 0
    %endif

    push_all

    ; Set stack pointer as function argument.
    ; esp indicate top address of saved context.
    push esp

    call %2

    add esp, 4

    pop_all

    ; Remove error code from stack.
    add esp, 4

    iretd
%endmacro


section .text

; making interrupt handler.
; first argument is 0, if interrupt has NOT error code.
; second argument is handler function in C.
make_interrupt_handler 0, interrupt_keybord
make_interrupt_handler 0, interrupt_mouse
make_interrupt_handler 0, interrupt_timer
make_interrupt_handler 1, exception_page_fault
