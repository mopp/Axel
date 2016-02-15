;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @file
; @brief Booting kernel main routine.
; @author mopp
; @version 0.2
; @date 2015-09-25
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
bits 32


; Multiboot header section.
; This header is read by multiboot bootstraps loader (E.g., grub2).
; And based on "The Multiboot Specification version 0.6.96"
; {{{
MULTIBOOT_HEADER_MAGIC     equ 0x1BADB002 ; The magic number identifying the header
MULTIBOOT_PAGE_ALIGN_BIT   equ 0x00000001 ; Align all boot modules on i386 page (4KB) boundaries.
MULTIBOOT_MEMORY_INFO_BIT  equ 0x00000002 ; Must pass memory information to OS.
MULTIBOOT_VIDEO_MODE_BIT   equ 0x00000004 ; Must pass video information to OS.
MULTIBOOT_BOOTLOADER_MAGIC equ 0x2BADB002 ; This must be set in %eax by bootloader.

section .multiboot_header
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd (MULTIBOOT_PAGE_ALIGN_BIT | MULTIBOOT_MEMORY_INFO_BIT | MULTIBOOT_VIDEO_MODE_BIT)
    dd -(MULTIBOOT_HEADER_MAGIC + (MULTIBOOT_PAGE_ALIGN_BIT | MULTIBOOT_MEMORY_INFO_BIT | MULTIBOOT_VIDEO_MODE_BIT))

    ; Address field (this fields are required in a.out format)
    dd $0
    dd $0
    dd $0
    dd $0
    dd $0

    ; Graphic field
    dd $1
    dd $80
    dd $25
    dd $0
; }}}


; Boot kernel section
; This section invoked by bootloader.
; Then, This calls kernel main routine.
; {{{
section .boot_kernel
align 4
global boot_kernel
extern main
extern LD_KERNEL_BSS_BEGIN
extern LD_KERNEL_BSS_END
boot_kernel:
    ; NOTE that We cannot use eax and ebx.
    ; Because eax and ebx has multiboot magic number and multiboot info struct addr.
    cli

    ; check multiboot magic number.
    ; If mismatched, goto sleep loop.
    cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
    jne sleep

    ; Clean up BSS section.
    mov edi, LD_KERNEL_BSS_BEGIN
    mov ecx, LD_KERNEL_BSS_END
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; Set kernel stack.
    mov esp, kernel_init_stack_top

    ; ebx is pointer to multiboot info struct.
    push ebx
    ; set ebx as pointer to argument array.
    push esp

    inc eax
    push eax

    call main

sleep:
    hlt
    jmp sleep
; }}}


; BSS (Block Started by Symbol) section
; This allocate initial kernel stack witch is 4KB.
; {{{
section .bss
align 4
KERNEL_INIT_STACK_SIZE equ 0x1000
kernel_init_stack_bottom:
    resb KERNEL_INIT_STACK_SIZE
kernel_init_stack_top:
; }}}
