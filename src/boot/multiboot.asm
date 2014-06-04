;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @file boot/multiboot.asm
; @brief It is called by grub and call kernel entry point.
; @author mopp
; @version 0.1
; @date 2014-05-21
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
bits 32


; for C Preprocessor.
#define _ASSEMBLY
#include <multiboot.h>
#include <paging.h>


extern kernel_entry


; multiboot header section.
; This is read by multiboot bootstraps loader (grub2).
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


; boot kernel section
; This section is entry point and called by bootstraps loader.
section .boot_kernel
global boot_kernel
boot_kernel:
    ; NOTE that We cannot use eax and ebx.
    ; eax and ebx has multiboot magic number and multiboot_info struct.
    cli

    ; check multiboot magic number.
    ; If mismatched, goto infinity sleep loop.
    cmp eax, MULTIBOOT_BOOTLOADER_MAGIC
    jne sleep

    ; Load page directory table.
    ; But, paging is NOT enable in this.
    ; So We have to calculate physical address from virtual address.
    mov ecx, (kernel_init_page_directory_table - KERNEL_VIRTUAL_BASE_ADDR)
    mov cr3, ecx

    ; Set PSE bit in CR4 to enable 4MB pages.
    mov ecx, cr4
    or  ecx, 0x00000010
    mov cr4, ecx

    ; Set PG bit in CR0 to enable paging.
    mov ecx, cr0
    or  ecx, 0x80000000
    mov cr0, ecx

    ; Paging is enable in this :)
    ; Virtual address 0x00000000~4M is required here.
    ; Then, We change eip for higher kernel, but eip has physical address yet.
    ; jump destination(boot_higher_kernel) exists virtual 0xC0000000~4MB address.
    ; So, We set eip = 0xCXXXXXXXX.
    ; And first page and kernel page is same mapping(0x00000000~4MB).
    lea ecx, [boot_higher_kernel]
    jmp ecx

boot_higher_kernel:
    ; Clean first page and its cache.
    ; It should not be needed anymore.
    mov dword [kernel_init_page_directory_table], 0
    invlpg [0]

    ; Kernel uses this stack.
    mov esp, kernel_init_stack_top

    ; ebx is pointer to multiboot_info.
    ; It is found while disable paging.
    ; So, fix address
    add  ebx, KERNEL_VIRTUAL_BASE_ADDR
    push ebx
    call kernel_entry

sleep:
    hlt
    jmp sleep


; date section.
; This is Page Directory Table.
; And Page size is 4MB in this.
section .data
align 0x1000
KERNEL_PAGE_INDEX   equ KERNEL_VIRTUAL_BASE_ADDR >> 22  ; Page directory index of kernel.
kernel_init_page_directory_table:
    ; We must have some page which is mapped to physical address 0x100000.
    ; Because it(temporaly paging) is required for enable paging.
    ; And This page is straight mapping virtual 0x00000000~4MB to phys 0x00000000~4MB and mapping virtual 0xC0000000~8MB to phys 0x00000000~8MB
    dd 0x00000083
    times (KERNEL_PAGE_INDEX - 1) dd 0                      ; Before kernel space.
    dd 0x00000083                                           ; Virtual 0xC0000000 map to physical 0x000000
    dd 0x00400083                                           ; Virtual 0xC0400000 map to physical 0x400000
    ; Later area should NOT refered by CPU.


; BSS(Block Started by Symbol) section
; This allocate initial kernel stack witch is 4KB.
section .bss
KERNEL_INIT_STACK_SIZE equ 0x1000
kernel_init_stack_bottom:
    resb KERNEL_INIT_STACK_SIZE
kernel_init_stack_top:
