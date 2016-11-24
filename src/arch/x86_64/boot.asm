;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @file
; @brief The codes boot the kernel.
;        Before entering these codes, The machine state must be 32-bit protected mode.
;        Reference:
;           [Setting Up Long Mode](http://wiki.osdev.org/Setting_Up_Long_Mode)
;           [Intel(R) 64 and IA-32 Architectures Software Developer's Manual]()
; @author mopp
; @version 0.2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 32


section .multiboot2

MULTIBOOT2_MAGIC equ 0xE85250D6
MULTIBOOT2_EAX_MAGIC equ 0x36D76289

; @brief
;   Multiboot header region.
multiboot2_begin: ; {{{
    dd MULTIBOOT2_MAGIC                  ; Magic number
    dd 0                                 ; Architecture (i386 32-bit protected mode)
    dd multiboot2_end - multiboot2_end   ; Header length
    ; Checksum, it subtracts from 0x100000000 to avoid compiler warning.
    dd 0x100000000 - (MULTIBOOT2_MAGIC + 0 + (multiboot2_end - multiboot2_end))
    ; The tag for terminating.
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot2_end:
; }}}



; Before turning on the paging, we cannot use jump instruction because the usual addresses of the symbols are linked at the higher kernel space.
; It means that we cannot use a function.
; However in order to use jump instruction, this 32bit section is mapped to only the load memory addresses.
section .32bit_text

; @brief
;   Axel starting function.
global start_axel
start_axel:
; {{{
    ; Clear interrupt.
    cli

    ; Set temporally stack.
    ; 0x500 - 0x1000 is free to use.
    mov esp, 0x1000

    ; Store the pointer to the multiboot information struct.
    push ebx

    ; Check eax has the correct multiboot2 magic number.
    test eax, MULTIBOOT2_EAX_MAGIC
    jz boot_failure

    call is_cpuid_available

    call is_sse_available
    call enable_sse

    call is_long_mode_available
    call enter_long_mode

    ; Load the pointer to the multiboot information struct.
    pop ebx

    ; In the long mode, paging is enable.
    ; The instruction pointer points to the kernel load address here.
    ; So, It have to be changed to the kernel virtual address.
    lea ecx, [.change_ip_register]
    jmp ecx

.change_ip_register:
    ; Set 64-bit Global Descriptor Table Register (GDTR)
    lgdt [gdtr64]

    ; Let's go to the 64bit world :)
    ; Set the code segment and enter 64-bit long mode.
    jmp gdt64.descriptor_code:enter_64bit_mode
; }}}


; @brief
;   If axel could not be execute, this function will be called.
boot_failure:
; {{{
    hlt
    jmp boot_failure
; }}}


; @brief
;   Check the CPUID instruction available or not.
;   The ID flag (bit 21) in the EFLAGS register indicates support for the CPUID instruction.
;   If this flag can be modified, it refers this CPU has the CPUID.
;   For more information, please refer "19.1 USING THE CPUID INSTRUCTION" in the Intel manual.
is_cpuid_available:
; {{{
    ; Store the original values in the EFLAGS.
    pushfd

    ; Set the invert value of the ID flag (bit 21).
    pushfd
    xor dword [esp], (1 << 21)
    popfd

    pushfd
    pop eax

    ; Compare the original EFLAGS and the current EFLAGS.
    xor eax, [esp]

    jz boot_failure

    ; Load the original EFLAGS.
    popfd

    ret
; }}}


; @brief
;   Check long mode (IA-32e mode) available or not.
;   Long mode is the name in the AMD64 and IA-32e mode is the name in the Intel 64.
is_long_mode_available:
; {{{
    ; Try to obtain Extended Function CPUID Information.
    mov eax, 0x80000000
    cpuid

    ; Now, eax has Maximum Input Value for Extended Function CPUID Information.
    ; If the maximum value is 0x80000000, it means this CPU don't have more extended functions.
    xor eax, 0x80000000
    jz boot_failure

    ; Check CPU extended functions.
    ; Bit 29 is 1 if Intel 64 Architecture is available.
    mov eax, 0x80000001
    cpuid

    test edx, (1 << 29)
    jz boot_failure

    ret
; }}}


; @brief
;   Check SSE extension is supported or not
is_sse_available:
; {{{
    ; Try to obtain Feature Information.
    mov eax, 0x1
    cpuid

    ; Check SSE extension.
    test edx, (1 << 25)
    jz boot_failure

    ret
; }}}


; @brief
;   Enable SSE extension.
;   For more Information, Please refer 9.6 INITIALIZING SSE/SSE2/SSE3/SSSE3 EXTENSIONS.
enable_sse:
; {{{

    ; Clear EM bit and set MP bit in CR0
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x2
    mov cr0, eax

    ; Set OSFXSR and OSXMMEXCPT bit in CR4.
    mov eax, cr4
    or ax, (3 << 9)
    mov cr4, eax

    ret
; }}}


; @brief Enter long mode (Compatibility mode in IA-32e mode).
;   1. Disable paging. (This is already done by the bootloader which complies with the multiboot2 specification.)
;   2. Enable physical-address extensions (PAE) by setting PAE bit in CR4.
;   3. Load CR3 with the physical base address of the Level 4 page map table (PML4).
;   4. Enable long mode (IA-32e mode) by setting IA32_EFER.LME = 1.
;   5. Enable paging.
; For more information, please refer 9.8.5 Initializing IA-32e Mode in the intel manual.
enter_long_mode:
; {{{
    ; Enable PAE.
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set the page struct address to CR3.
    ; 64-bit mode paging tables must be located in the first 4 GBytes of physical-address space prior to activating IA-32e mode.
    mov edi, 0x1000
    mov cr3, edi

    ; Clean up the memories for the paging.
    xor eax, eax
    mov ecx, (0x5000 - 0x1000) / 4
    rep stosd

    ; Configure the level4, level3 and level2 entries.
    ;   Level4 Table/Entry - Page Map Level 4 Table/Entry
    ;   Level3 Table/Entry - Page Directory Pointer Table/Entry
    ;   Level2 Table/Entry - Page Directory Table/Entry
    ;   Level1 Table/Entry - Page Table/Entry
    ; Each mapping region is 2MB for the kernel load address and the kernel virtual address.
    ; For more information, Please refer 4.5 IA-32E PAGING in the intel manual.
    mov dword [0x1000], 0x00002003 ; Set the level4 entry.
    mov dword [0x2000], 0x00003003 ; Set the level3 entry for the kernel load address.
    mov dword [0x2008], 0x00004003 ; Set the level3 entry for the kernel virtual address.
    mov dword [0x3000], 0x00000083 ; Set the level2 entry for the kernel load address.
    mov dword [0x4000], 0x00000083 ; Set the level2 entry for the kernel virtual address.

    ; Set the long mode bit in the EFER MSR.
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging.
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret
; }}}



bits 64


section .64bit_text

; @brief Enter 64-bit mode in IA-32e mode.
enter_64bit_mode:
; {{{
    ; Invalidate the entry for the kernel load address.
    ; It is never used in the long mode.
    mov dword [0x3000], 0
    invlpg [0]

    ; Set the segment registers.
    mov ax, gdt64.descriptor_data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, kernel_stack_top

    ; rbx is pointer to multiboot info struct.
    extern KERNEL_ADDR_VIRTUAL_BASE
    lea rax, [KERNEL_ADDR_VIRTUAL_BASE]
    add rbx, rax

    push rbx

    ; Set the arguments of the main function.
    mov rdi, 1
    mov rsi, rsp

extern main
    call main

    hlt
; }}}


section .bss

KERNEL_STACK_SIZE_BYTE equ 0x1000

global kernel_stack_top
kernel_stack_bottom:
;{{{
    resb KERNEL_STACK_SIZE_BYTE
kernel_stack_top:
; }}}


section .rodata

; Global descriptor table in 64-bit mode.
; Note that the CPU does not perform segment limit checks at runtime in 64-bit mode.
align 8
gdt64:
; {{{
    .descriptor_null: equ $ - gdt64
    dd 0x00000000
    dd 0x00000000
    .descriptor_code: equ $ - gdt64
    dd 0x00000000
    dd 0x00209A00
    .descriptor_data: equ $ - gdt64
    dd 0x00000000
    dd 0x00009200

; Global descriptor table register in 64-bit mode.
; For more information, please refer 3.5.1 Segment Descriptor Tables in the intel manual.
gdtr64:
    dw $ - gdt64 - 1
    dq gdt64
; }}}
