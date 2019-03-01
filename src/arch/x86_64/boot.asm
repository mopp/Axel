;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @brief
;   Program to boot the kernel.
;   This program have to be called from a bootloader complying multiboot2 spec.
;   Reference:
;      [Memory Map (x86)](http://wiki.osdev.org/Memory_Map_(x86))
;      [Setting Up Long Mode](http://wiki.osdev.org/Setting_Up_Long_Mode)
;      [Intel(R) 64 and IA-32 Architectures Software Developer's Manual]()
;      [Canonical form addresses](https://en.wikipedia.org/wiki/X86-64#Virtual_address_space_details)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 32

; This section is for the Multiboot header.
; It must be located in the first 32768 bytes of the OS binary.
section .multiboot2
; {{{

; The multiboot magic numbers.
MULTIBOOT2_MAGIC     equ 0xE85250D6
MULTIBOOT2_EAX_MAGIC equ 0x36D76289

; @brief
;   Multiboot header region.
align 8
multiboot2_begin: ; {{{
    dd MULTIBOOT2_MAGIC                  ; Magic number
    dd 0                                 ; Architecture (i386 32-bit protected mode)
    dd multiboot2_end - multiboot2_begin ; Header length
    ; Checksum, it subtracts from 0x100000000 to avoid compiler warning.
    dd 0x100000000 - (MULTIBOOT2_MAGIC + 0 + (multiboot2_end - multiboot2_begin))
    ; The tag for terminating.
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot2_end:
; }}}
; }}}



; This section is for configure the CPU.
; Before turning on the paging, we cannot use some instructions such as jmp and call, which want to use 64 bit addresses.
; Because we are in 32-bit protected mode still here.
; However, the symbols are linked at the higher kernel space (over 32-bit).
; Then, in order to use the instructions, this section is mapped to the load memory addresses only using linker script.
section .text_boot_32bit
; {{{

; This constant refers the virtual kernel address offset.
; When I tried to read this values using the linker script, an error `relocation truncated to fit: R_X86_64_32 against ~~~` was ommited.
; Therefore, I had to write the value directly here :(
KERNEL_ADDR_VIRTUAL_OFFSET equ 0xFFFF800000000000

; @brief
;   Axel starting function.
global start_axel
start_axel:
; {{{
    ; Clear interrupt.
    cli

    ; Set temporal stack.
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
    call enter_compatibility_mode

    ; Load the pointer to the multiboot information struct.
    pop ebx

    call enter_64bit_mode

    jmp prelude_to_canonical_higher_harf
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


; @brief
;   Enter compatibility mode (submode of IA-32e mode).
;       1. Disable paging. (This is already done by the bootloader which complies with the multiboot2 specification.)
;       2. Enable physical-address extensions (PAE) by setting PAE bit in CR4.
;       3. Load CR3 with the physical base address of the Level 4 page map table (PML4).
;       4. Enable long mode (IA-32e mode) by setting IA32_EFER.LME = 1.
;       5. Enable paging.
;   For more information, please refer 9.8.5 Initializing IA-32e Mode in the intel manual.
enter_compatibility_mode:
; {{{
    ; Enable PAE.
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Set the page struct address to CR3.
    ; 64-bit mode paging tables must be located in the first 4 GBytes of physical-address space prior to activating IA-32e mode.
    ; The memory region 0x00500 ~ 0x7FFFF can be used.
    mov edi, 0x1000
    mov cr3, edi

    ; Clean up the memories for the paging.
    xor eax, eax
    mov ecx, (0x4000 - 0x1000) / 4
    rep stosd

    ; Configure temporal page settings.
    ;   Level4 Table/Entry - Page Map Level 4 Table/Entry
    ;   Level3 Table/Entry - Page Directory Pointer Table/Entry
    ;   Level2 Table/Entry - Page Directory Table/Entry
    ;   Level1 Table/Entry - Page Table/Entry
    ; Each mapping region is 1GB for the kernel load address and the kernel virtual address.
    ; For more information, Please refer 4.5 IA-32E PAGING in the intel manual.

    ; Entries for the kernel load address.
    mov dword [0x1000 + 8 *   0], 0x00002003 ; Set the level4 entry.
    mov dword [0x2000 + 8 *   0], 0x00000083 ; Set the level3 entry

    ; Entries for the kernel virtual address.
    mov dword [0x1000 + 8 * 256], 0x00003003 ; Set the level4 entry.
    mov dword [0x3000 + 8 *   0], 0x00000083 ; Set the level3 entry.

    ; Entry for the recursive page mapping.
    mov dword [0x1000 + 8 * 511], 0x00001003 ; Set the level4 entry.

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


; @brief Enter 64-bit mode (submode of IA-32e mode).
;   Set GDT for 64-bit.
;   Code segment-descriptor should be set correctly.
;   For more information, please refer 9.8.5.3 64-bit Mode and Compatibility Mode Operation in the intel manual.
enter_64bit_mode:
; {{{
    ; Load 64-bit Global Descriptor Table Register (GDTR)
    mov eax, gdtr64 - KERNEL_ADDR_VIRTUAL_OFFSET
    lgdt [eax]

    ; Change the code segment register.
    jmp gdt64.descriptor_kernel_code:.change_segment_register

.change_segment_register:
    ; Set the segment registers.
    mov ax, gdt64.descriptor_kernel_data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 64-bit mode is already available here,
    ; So, the ret instruction try to load 8bytes (not 4bytes) from the stack to return.
    ; However, this function was called from 32-bit world.
    ; We have to load correctly 4bytes.
    mov eax, dword [esp]
    add esp, 4
    jmp eax
; }}}
; }}}



bits 64

; This section is for 64-bit instruction, and it is located at the kernel load address.
; The purpose of this section is to jump canonical higher harf space because the address is 64-bit.
section .text_boot_64bit
; {{{

; @brief
;   Jump via register.
;   Because direct jump cause an error 'relocation truncated to fit: R_X86_64_PC32 against `.text_canonical_higher_harf''
prelude_to_canonical_higher_harf:
; {{{
    mov rax, canonical_higher_harf

    ; Let's go to the canonical higher harf space :)
    jmp rax
; }}}
; }}}



; This section is mapped to the kernel virtual address.
section .text_canonical_higher_harf
; {{{

; 0x00007E00 - 0x0007FFFF is free region.
; See x86 memory map.
KERNEL_STACK_FRAME_COUNT equ 4
KERNEL_STACK_ADDR_TOP    equ 0x00070000
KERNEL_STACK_ADDR_BOTTOM equ KERNEL_STACK_ADDR_TOP + (4096 * KERNEL_STACK_FRAME_COUNT) - 1

%if 0x0007FFFF < KERNEL_STACK_ADDR_BOTTOM
%error "Kernel stack is out of range"
%endif

; @brief
;   Configure some information should be passed to the main function.
;   Then, call the main function.
canonical_higher_harf:
; {{{
    ; Invalidate the entry for the kernel load address.
    ; It is never used in the long mode.
    mov dword [0x1000], 0
    invlpg [0]

    ; Reload GDTR
    ; The prevous GDTR is not 64 bit address.
    mov rax, gdtr64
    lgdt [rax]

    mov rax, KERNEL_ADDR_VIRTUAL_OFFSET

    ; Set the kernel stask.
    ; The harf will be used as a guard page.
    mov rcx, KERNEL_STACK_ADDR_BOTTOM
    add rcx, rax
    mov rsp, rcx

    ; Set the number of pages for kernel stack.
    push KERNEL_STACK_FRAME_COUNT

    ; Set kernel stack address top
    mov rcx, KERNEL_STACK_ADDR_TOP
    push rcx

    ; rbx is pointer to multiboot info struct.
    add rbx, rax
    push rbx

    ; Set the arguments of the main function.
    mov rdi, 3
    mov rsi, rsp

    extern main
    call main

.loop:
    hlt
jmp .loop
; }}}
; }}}



section .rodata
; {{{

; Global descriptor table in 64-bit mode.
; Note that the CPU does not perform segment limit checks at runtime in 64-bit mode.
; `$ - gdt64` makes each label refers to segment selector value.
align 8
gdt64:
; {{{
    .descriptor_null: equ $ - gdt64
    dd 0x00000000
    dd 0x00000000
    .descriptor_kernel_code: equ $ - gdt64
    ; Set 64-bit flag, present flag.
    ; Execute and read.
    ; DPL is 0.
    dd 0x00000000
    dd 0x00209A00
    .descriptor_kernel_data: equ $ - gdt64
    ; Set 64-bit flag, present flag.
    ; Read and write.
    ; DPL is 0.
    dd 0x00000000
    dd 0x00009200
    .descriptor_user_code: equ $ - gdt64
    ; DPL is 3.
    dd 0x00000000
    dd 0x0020FA00
    .descriptor_user_data: equ $ - gdt64
    ; DPL is 3.
    dd 0x00000000
    dd 0x0000F200
; }}}

; Global descriptor table register in 64-bit mode.
; For more information, please refer 3.5.1 Segment Descriptor Tables in the intel manual.
gdtr64:
; {{{
    dw $ - gdt64 - 1
    dq gdt64
; }}}
; }}}
