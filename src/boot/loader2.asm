;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @file loader2.asm
; @brief Second OS boot loader
; @author mopp
; @version 0.1
; @date 2015-02-06
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


global loader2_interrupt_timer
loader2_interrupt_timer:
extern interrupt_timer
    pushad
    call interrupt_timer
    popad
    iret
