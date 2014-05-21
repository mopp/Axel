;;
; vim:ft=nasm:foldmethod=marker
; @file boot/multiboot.asm
; @brief It is called by grub and call kernel entry point.
; @author mopp
; @version 0.1
; @date 2014-05-21
;;
bits 32

#define _ASSEMBLY
#include <multiboot.h>
#include <paging.h>


KERNEL_PAGE_INDEX          equ (KERNEL_VIRTUAL_BASE_ADDR >> 22)     ; Page directory index of kernel s 4MB PTE.


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


section .boot_kernel
    extern kernel_entry
    global boot_kernel
boot_kernel:
    cli

    cmp EAX, MULTIBOOT_BOOTLOADER_MAGIC
    jne sleep

    mov ecx, (kernel_init_page_directory - KERNEL_VIRTUAL_BASE_ADDR)
    mov cr3, ecx                                ; Load Page Directory Base Register.

    mov ECX, CR4
    or  ECX, 0x00000010                         ; Set PSE bit in CR4 to enable 4MB pages.
    mov CR4, ECX

    mov ECX, CR0
    or  ECX, 0x80000000                         ; Set PG bit in CR0 to enable paging.
    mov CR0, ECX

    lea ECX, [higher_half]
    jmp ECX

higher_half:

.exit:
    mov dword [kernel_init_page_directory], 0

    ; hlt
    ; jmp .exit

    ; invlpg [0]

    mov ESP, stack_top

    ; ブート情報構造体の格納アドレスを引数へ
    add  EBX, KERNEL_VIRTUAL_BASE_ADDR
    push EBX
    call kernel_entry
sleep:
    hlt
    jmp sleep


section .data
align 0x1000
VBE_PAGE_INDEX  equ (0xFD000000 >> 22)
kernel_init_page_directory:
    dd 0x00000083
    times (KERNEL_PAGE_INDEX - 1) dd 0                 ; Pages before kernel space.
    dd 0x00000083
    times (VBE_PAGE_INDEX - KERNEL_PAGE_INDEX - 1) dd 0
    dd 0xFD000083
    times (1024 - VBE_PAGE_INDEX - 1) dd 0          ; Pages after the kernel image.


section .bss
KERNEL_INIT_STACK_SIZE equ 0x4000
stack_bottom:
    resb KERNEL_INIT_STACK_SIZE
stack_top:
