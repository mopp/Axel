;---------------------------------------------------------------------
; vim:ft=nasm:foldmethod=marker
; File: src/interrupt_handler.asm
; Description: It provides interrupt handler entry point
;---------------------------------------------------------------------
    bits 32

%macro push_all 0
        push DS
        push ES
        push FS
        push GS
        pushad
        pushfd
%endmacro


%macro pop_all 0
        popfd
        popad
        pop GS
        pop FS
        pop ES
        pop DS
%endmacro


; 割り込みハンドラのひな形
%macro asm_interrupt_handler 1
    global asm_interrupt_handler%1
    extern interrupt_handler%1

asm_interrupt_handler%1:
    push_all

    call interrupt_handler%1

    pop_all

    iretd
%endmacro


section .text

; 割り込みハンドラを作成
asm_interrupt_handler 0x20
asm_interrupt_handler 0x21
