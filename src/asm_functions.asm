;---------------------------------------------------------------------
; vim:ft=nasm:foldmethod=marker
; File: src/asm_functions.asm
; Description:
;       It provides some assembly only functions to call those in C.
;---------------------------------------------------------------------
    bits 32

; for C Preprocessor.
#define _ASSEMBLY_H_
#include <paging.h>
#include <segment.h>

    global io_hlt, io_cli, io_sti
    global io_in8, io_in16, io_in32
    global io_out8, io_out16, io_out32
    global load_gdtr, load_idtr
    global turn_off_4MB_paging, set_cpu_pdt, get_cpu_pdt, flush_tlb, flush_tlb_all, turn_on_pge, turn_off_pge, load_cr2
    global set_task_register, get_task_register


section .text


; void io_hlt(void);
io_hlt:
    hlt
    ret


; void io_cli(void);
io_cli:
    cli
    ret


; void io_sti(void);
io_sti:
    sti
    ret


; uint8_t io_in8(uint16_t port);
io_in8:
    mov EDX, [ESP + 4] ; port
    mov EAX, 0
    in  AL, DX
    ret


; uint16_t io_in16(uint16_t port);
io_in16:
    mov EDX, [ESP + 4] ; port
    mov EAX, 0
    in  AX, DX
    ret


; uint32_t io_in32(uint16_t port);
io_in32:
    mov EDX, [ESP + 4] ; port
    mov EAX, 0
    in  EAX, DX
    ret


; void io_out8(uint16_t port, uint8_t data);
io_out8:
    mov EDX, [ESP + 4]  ; port
    mov AL, [ESP + 8]   ; data
    out DX, AL
    ret


; void io_out16(uint16_t port, uint16_t data);
io_out16:
    mov EDX, [ESP + 4]  ; port
    mov EAX, [ESP + 8]  ; data
    out DX, AX
    ret


; void io_out32(uint16_t port, uint32_t data);
io_out32:
    mov EDX, [ESP + 4]  ; port
    mov EAX, [ESP + 8]  ; data
    out DX, EAX
    ret


; void load_gdtr(uint32_t limit_byte, uint32_t addr);
; 大きさが2byte アドレスが4byte 合わせて6byteをlgdtが読み込む
; なので、limit_byteの下位2byteをずらしている。 ldtrも同様
load_gdtr:
    mov ax, [esp + 4]      ; limit
    mov [esp + 6], ax
    lgdt [esp + 6]
    jmp KERNEL_CODE_SEGMENT_SELECTOR:.change_cs
.change_cs:
    mov ax, KERNEL_DATA_SEGMENT_SELECTOR
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret


; void load_idtr(int limit_byte, int addr);
load_idtr:
    mov AX, [ESP + 4]      ; limit
    mov [ESP + 6], AX
    lidt [ESP + 6]
    ret


; void turn_off_4MB_paging(void);
turn_off_4MB_paging:
    mov ecx, cr4
    and ecx, 0xFFFFFFEF
    mov cr4, ecx
    ret


; void turn_off_pge(void);
turn_off_pge:
    mov ecx, cr4
    and ecx, 0xFFFFFF7F
    mov cr4, ecx
    ret


; void turn_on_pge(void);
turn_on_pge:
    mov ecx, cr4
    or  ecx, 0x00000080
    mov cr4, ecx
    ret


; void set_cpu_pdt(Page_directory_table pdt);
; address that is set into cr3 should be physical address.
set_cpu_pdt:
    mov ebx, [esp + 4]
    sub ebx, KERNEL_VIRTUAL_BASE_ADDR;
    mov cr3, ebx
    ret


; Page_directory_table get_cpu_pdt(void);
get_cpu_pdt:
    mov eax, cr3
    add eax, KERNEL_VIRTUAL_BASE_ADDR
    ret


; void flush_tlb(uintptr_t addr);
flush_tlb:
    cli
    invlpg [esp + 4]
    sti
    ret


; void flush_tlb_all(void);
flush_tlb_all:
    mov eax, cr3
    mov cr3, ebx
    ret


; uintptr_t load_cr2(void);
load_cr2:
    mov eax, cr2
    ret


; void set_task_register(uint16_t seg_selector);
set_task_register:
    mov ax, [esp + 4]
    ltr ax
    ret

; uint16_t get_task_register(void);
get_task_register:
    or  eax, 0
    str ax
    ret
