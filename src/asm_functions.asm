;---------------------------------------------------------------------
; vim:ft=nasm:foldmethod=marker
; File: src/asm_functions.asm
; Description:
;       It provides some assembly only functions to call those in C.
;---------------------------------------------------------------------
    bits 32

    global io_hlt, io_cli, io_sti
    global io_in8, io_in16, io_in32
    global io_out8, io_out16, io_out32
    global load_gdtr, load_idtr
    global change_segment_selectors

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
    mov AX, [ESP + 4]      ; limit
    mov [ESP + 6], AX
    lgdt [ESP + 6]
    ret


; void load_idtr(int limit_byte, int addr);
load_idtr:
    mov AX, [ESP + 4]      ; limit
    mov [ESP + 6], AX
    lidt [ESP + 6]
    ret

; void change_segment_selectors(int data_segment);
change_segment_selectors:
    jmp 0x08:.jmp_point
.jmp_point:
    mov AX, [ESP + 4]
    mov ES, AX
    mov DS, AX
    mov FS, AX
    mov GS, AX
    mov SS, AX
    ret
