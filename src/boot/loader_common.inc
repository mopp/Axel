;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @file boot/loader_common.inc
; @brief This is bootstrap header.
; @author mopp
; @version 0.1
; @date 2014-11-01
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Macros
%macro putchar 1 ; {{{
    mov al, %1
    mov ah, 0x0E
    int 0x10
%endmacro
; }}}


%macro newline 0 ; {{{
    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
%endmacro
; }}}


%macro puts 1 ; {{{
    mov si, %1
    call print_string
%endmacro
; }}}


; Sub routines
error: ; {{{
    puts str_key_wait

    xor ax, ax
    int 0x16
    int 0x19 ; reboot
str_key_wait:    db 'Please press any key to reboot.', 0
; }}}


print_string: ; {{{
    push ax
    mov ah, 0x0E
.loop:
    lodsb       ; Grab a byte from SI
    or al, al   ; Check for 0
    jz .done

    int 0x10

    jmp .loop
.done:
    newline

    pop ax
    ret
;}}}
