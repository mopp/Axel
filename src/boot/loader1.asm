;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim:ft=nasm:foldmethod=marker
; @file boot/load1.asm
; @brief This is primary bootstrap loader in MBR.
; @author mopp
; @version 0.1
; @date 2014-11-01
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;---------------------------------------------------------------------
; - Memo -
; CPU mode : 8086 compatible (16bit)
; read 0000:7C00
; This program NOT includes partition block.
;
; Segment register:Byte-address
; Segment register * 16(0x10) + offset = address.
; 0x07C0 * 16 + 0x0000= 0x7C00
;
; First machine memory map is here.
;
;                     Axel Memory maps
; 0x00000000 +---------------+----------------+
;            |       Interrupt vector         |
;            |             table              |
; 0x00000400 +--------------------------------+
;            |           BIOS data            |
; 0x00000500 +--------------------------------+
;            |                                |
;            ~          unused area           ~
;            |                                |
; 0x00007C00 +--------------------------------+
;            |      Boot loader(loader1)      |
; 0x00007E00 +--------------------------------+
;            |                                |
;            |                                |
;            ~          unused area           ~
;            |                                |
;            |                                |
; 0x000A0000 +--------------------------------+
;            |                                |
;            |          Video memory          |
;            |             VRAM               |
;            |                                |
; 0x000B0000 +--------------------------------+
;            |                                |
;            |        monochrome VRAM         |
;            |                                |
; 0x000B8000 +--------------------------------+
;            |          color VRAM            |
; 0x000C0000 +--------------------------------+
;            |                                |
;            |        BIOS video area         |
;            |                                |
; 0x000C8000 +--------------------------------+
;            |                                |
;            |         BIOS reserved          |
;            |                                |
; 0x000F0000 +--------------------------------+
;            |                                |
;            |             BIOS               |
;            |                                |
;            |                                |
; 0x00100000 +--------------------------------+
;
;
; In above, 0x0500 - 0xA0000 is free.
; After, this program(at the moment jumping second loader) map is here.
;
; 0x00000500 +--------------------------------+
;            |            loader1             |
; 0x00001700 +--------------------------------+
;            |                                |
;            ~          unused area           ~
;            |                                |
; 0x00007C00 +--------------------------------+
;            |            loader2             |
; 0x00007E00 +--------------------------------+
;            |                                |
;            |                                |
;            ~          unused area           ~
;            |                                |
;            |                                |
; 0x000A0000 +--------------------------------+
;---------------------------------------------------------------------


; Constant
; These do not allocate in memory.
; And expand at compile time.
; {{{
MBR_SIZE         equ 512
STACK_TOP        equ 0x500
STACK_SIZE       equ 0x1000
STACK_BOTTOM     equ STACK_TOP + STACK_SIZE
RELOCATE_ADDR    equ STACK_BOTTOM
LOAD_ADDR        equ 0x7C00
NEXT_LOADER_ADDR equ LOAD_ADDR
PART_TABLE_ADDR  equ RELOCATE_ADDR + MBR_SIZE - 66
PART_ENTRY_SIZE  equ 16
BOOTABLE_FLAG    equ 0x80
; }}}


bits 16
org RELOCATE_ADDR


; jump start code.
; It must be here before any data.
jmp boot_loader1


%include "loader_common.inc"


; Start loader1
; DL is boot disk number
; {{{
boot_loader1:
    cli

    ; Setting all segment selector.
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov sp, STACK_BOTTOM

    ; Copy this mbr from LOAD_ADDR to RELOCATE_ADDR.
    mov cx, MBR_SIZE
    mov si, LOAD_ADDR
    mov di, RELOCATE_ADDR
    rep movsb

    ; Calculate destination address.
    ; And push segment and addr to jump.
    push es
    push RELOCATE_ADDR + (move - $$)
    retf

move:
    puts str_start

    ; Reset disk system.
    push dx
    xor ax, ax
    int 0x13
    pop dx

    jc error

    ; Scan partition entries.
    mov bx, PART_TABLE_ADDR

.find_bootable:
    cmp byte [bx], BOOTABLE_FLAG
    je load_loader2

    add bx, PART_ENTRY_SIZE
    loopne .find_bootable

    ; Nothing bootable drive.
    puts str_nothing_dev
    jmp error

; Load next sector.
load_loader2:
    ; In this, bx has bootable partition entry addr.
    push bx

    ; Check extend int 0x13 instruction.
    ; The carry flag will be set if Extensions are not supported.
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jmp error

    pop bx

    ; Setting disk address packet.
    mov word [disk_addr_pack + 0x6], cs
    ; first sector number(LBA) of boot partition
    mov eax, dword [bx + 0x08]
    mov [disk_addr_pack + 0x08], eax

    ; Clear ax for avoiding int 0x42 return code is broken.
    xor ax, ax

    ; Read sector.
    mov si, disk_addr_pack
    mov ah, 0x42
    int 0x13

    ; Check return code
    cmp al, 0x00
    jne error

    ; Jump to second loader.
    jmp NEXT_LOADER_ADDR
; }}}


; Data
; {{{
str_nothing_dev: db 'Nothing bootable device', 0
str_start:       db 'Start axel loader1', 0

; Disk address packet structure
; Offset Byte Size Description
; 0              1  Size of packet (16 bytes)
; 1              1  Always 0
; 2              2  The number of sectors to transfer (max 127 on some BIOSes)
; 4              4  Transfer buffer address(splited by 16 bit segment:16 bit offset)
;   4              2  segment offset
;   6              2  segment
; 8              4  Starting LBA
; 12             4  Used for upper part of 48 bit LBAs
align 4
disk_addr_pack:
    db 0x10
    db 0x00
    dw 0x0001           ; Load only 1 sector.
    dw NEXT_LOADER_ADDR ; Destination segment offset
    dw 0x00000000       ; Destination segment
    dd 0x00000000       ; This field is dynamicaly set by loader.
    dd 0x00000000
; }}}


; Fill the remain of area
times (MBR_SIZE - 2) - ($ - $$) db 0
dw 0xAA55
