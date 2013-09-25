;---------------------------------------------------------------------
; vim:ft=nasm:foldmethod=marker
; File: src/interrupt_handler.asm
; Description:
;       It provides interrupt handler entry point
;---------------------------------------------------------------------
    bits 32

    global asm_interrupt_handler0x20
    extern interrupt_handler0x20

section .text

%macro  handler_template 1
    push    ES
    push    DS
    pushad
    mov     EAX,ESP
    push    EAX
    mov     AX,SS
    mov     DS,AX
    mov     ES,AX
    call    %1
    pop     EAX
    popad
    pop     DS
    pop     ES
    iretd
%endmacro


asm_interrupt_handler0x20:
    handler_template(interrupt_handler0x20)
