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


%macro function 1
    global %1
    %1:
%endmacro


section .text


; void io_hlt(void);
function io_hlt
    hlt
    ret


; void io_cli(void);
function io_cli
    cli
    ret


; void io_sti(void);
function io_sti
    sti
    ret


; uint8_t io_in8(uint16_t port);
function io_in8
    mov EDX, [ESP + 4] ; port
    mov EAX, 0
    in  AL, DX
    ret


; uint16_t io_in16(uint16_t port);
function io_in16
    mov EDX, [ESP + 4] ; port
    mov EAX, 0
    in  AX, DX
    ret


; uint32_t io_in32(uint16_t port);
function io_in32
    mov EDX, [ESP + 4] ; port
    mov EAX, 0
    in  EAX, DX
    ret


; void io_out8(uint16_t port, uint8_t data);
function io_out8
    mov EDX, [ESP + 4]  ; port
    mov AL, [ESP + 8]   ; data
    out DX, AL
    ret


; void io_out16(uint16_t port, uint16_t data);
function io_out16
    mov EDX, [ESP + 4]  ; port
    mov EAX, [ESP + 8]  ; data
    out DX, AX
    ret


; void io_out32(uint16_t port, uint32_t data);
function io_out32
    mov EDX, [ESP + 4]  ; port
    mov EAX, [ESP + 8]  ; data
    out DX, EAX
    ret


; void load_gdtr(uint32_t limit_byte, uint32_t addr);
; 大きさが2byte アドレスが4byte 合わせて6byteをlgdtが読み込む
; なので、limit_byteの下位2byteをずらしている。 ldtrも同様
function load_gdtr
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
function load_idtr
    mov AX, [ESP + 4]      ; limit
    mov [ESP + 6], AX
    lidt [ESP + 6]
    ret


; void turn_off_4MB_paging(void);
function turn_off_4MB_paging
    mov ecx, cr4
    and ecx, 0xFFFFFFEF
    mov cr4, ecx
    ret


; void turn_off_pge(void);
function turn_off_pge
    mov ecx, cr4
    and ecx, 0xFFFFFF7F
    mov cr4, ecx
    ret


; void turn_on_pge(void);
function turn_on_pge
    mov ecx, cr4
    or  ecx, 0x00000080
    mov cr4, ecx
    ret


; void set_cpu_pdt(Page_directory_table pdt);
; address that is set into cr3 should be physical address.
function set_cpu_pdt
    mov ebx, [esp + 4]
    mov cr3, ebx
    ret


; Page_directory_table get_cpu_pdt(void);
function get_cpu_pdt
    mov eax, cr3
    ret


; void flush_tlb(uintptr_t addr);
function flush_tlb
    ; cli
    invlpg [esp + 4]
    ; sti
    ret


; void flush_tlb_all(void);
function flush_tlb_all
    mov eax, cr3
    mov cr3, ebx
    ret


; uintptr_t load_cr2(void);
function load_cr2
    mov eax, cr2
    ret


; void set_task_register(uint16_t seg_selector);
function set_task_register
    mov ax, [esp + 4]
    ltr ax
    ret

; uint16_t get_task_register(void);
function get_task_register
    or  eax, 0
    str ax
    ret
