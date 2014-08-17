/************************************************************
 * File: include/interrupt.h
 * Description: Interrupt Handler Header
 ************************************************************/

#ifndef _INTERRUPT_HANDLER_H_
#define _INTERRUPT_HANDLER_H_



#include <stdint.h>

/* Programmable Interrupt Controller */
enum PIC_constants {
    PIC0_CMD_STATE_PORT  = 0x20,
    PIC0_IMR_DATA_PORT   = 0x21,
    PIC1_CMD_STATE_PORT  = 0xA0,
    PIC1_IMR_DATA_PORT   = 0xA1,

    /* Initialization Control Words */
    PIC0_ICW1            = 0x11, /* PICの設定 */
    PIC0_ICW2            = 0x20, /* IRQから割り込みベクタへの変換されるベースの番号
                                  * x86では3-7bitを使用する
                                  * この番号にPICのピン番号が加算された値の例外ハンドラが呼び出される
                                  */
    PIC0_ICW3            = 0x04, /* スレーブと接続されているピン */
    PIC0_ICW4            = 0x01, /* 動作モードを指定 */

    PIC1_ICW1            = PIC0_ICW1,
    PIC1_ICW2            = 0x28,
    PIC1_ICW3            = 0x02,
    PIC1_ICW4            = PIC0_ICW4,

    /* Operation Command Words2 */
    PIC_OCW2_EOI         = 0x20,

    PIC_IMR_MASK_IRQ00   = 0x0001,
    PIC_IMR_MASK_IRQ01   = 0x0002,
    PIC_IMR_MASK_IRQ02   = 0x0004,
    PIC_IMR_MASK_IRQ03   = 0x0008,
    PIC_IMR_MASK_IRQ04   = 0x0010,
    PIC_IMR_MASK_IRQ05   = 0x0020,
    PIC_IMR_MASK_IRQ06   = 0x0040,
    PIC_IMR_MASK_IRQ07   = 0x0080,

    PIC_IMR_MASK_IRQ08   = 0x0100,
    PIC_IMR_MASK_IRQ09   = 0x0200,
    PIC_IMR_MASK_IRQ10   = 0x0400,
    PIC_IMR_MASK_IRQ11   = 0x0800,
    PIC_IMR_MASK_IRQ12   = 0x1000,
    PIC_IMR_MASK_IRQ13   = 0x2000,
    PIC_IMR_MASK_IRQ14   = 0x4000,
    PIC_IMR_MASK_IRQ15   = 0x8000,

    PIC_IMR_MASK_IRQ_ALL = 0xFF,
};


/*
 * This is stack structure when happen interruption by CPU.
 * Please see ./interrupt_asm.asm
 */
struct interrupt_context {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint16_t gs;
    uint16_t fs;
    uint16_t es;
    uint16_t ds;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};
typedef struct interrupt_context Interrupt_context;


extern void init_pic(void);
extern void enable_interrupt(uint16_t);
extern void disable_interrupt(uint16_t);
extern void send_done_interrupt_master(void);
extern void send_done_interrupt_slave(void);
extern void asm_interrupt_timer(void);
extern void asm_interrupt_keybord(void);
extern void asm_interrupt_mouse(void);



#endif
