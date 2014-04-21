;---------------------------------------------------------------------
; vim:ft=nasm:foldmethod=marker
; File: boot/multiboot.asm
; Description: It is called by grub and call kernel entry point.
;---------------------------------------------------------------------
    bits 32

#include <multiboot_constants.h>

; 上記定数を用いてヘッダを書き込み
section .multiboot_header
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd (MULTIBOOT_PAGE_ALIGN_BIT | MULTIBOOT_MEMORY_INFO_BIT | MULTIBOOT_VIDEO_MODE_BIT)
    dd -(MULTIBOOT_HEADER_MAGIC + (MULTIBOOT_PAGE_ALIGN_BIT | MULTIBOOT_MEMORY_INFO_BIT | MULTIBOOT_VIDEO_MODE_BIT))

    ; Address field
    dd $0
    dd $0
    dd $0
    dd $0
    dd $0

    ; Graphic field
    dd GRAPHIC_FIELD_MODE
    dd DISPLAY_X_RESOLUTION
    dd DISPLAY_Y_RESOLUTION
    dd DISPLAY_BIT_SIZE

section .kernel_init_stack
stack_bottom:
    ; times 16384 db 0
    times 16384 db 0
stack_top:


section .text
    extern kernel_entry

    ; リンク時にboot_kernelをエントリポイントとして指定すること
    global boot_kernel
boot_kernel:
    cli

    cmp EAX, MULTIBOOT_BOOTLOADER_MAGIC
    jne .sleep

    mov esp, stack_top

    ; ブート情報構造体の格納アドレスを引数へ
    push EBX
    call kernel_entry

    ; sti
.sleep:
    hlt
    jmp .sleep
