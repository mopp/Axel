/**
 * @file include/interrupt.h
 * @brief Interrupt handler header
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _INTERRUPT_HANDLER_H_
#define _INTERRUPT_HANDLER_H_



#include <stdint.h>


/* Programmable Interrupt Controller */
enum {
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
struct interrupt_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint16_t gs, reserved1;
    uint16_t fs, reserved2;
    uint16_t es, reserved3;
    uint16_t ds, reserved4;
    uint32_t error_code;
    uint32_t eip;
    uint16_t cs, reserved5;
    uint32_t eflags;
    uint32_t prev_esp;
    uint16_t prev_ss, reserved6;
};
typedef struct interrupt_frame Interrupt_frame;


struct process;
typedef struct process Process;

extern void init_idt(void);
extern void enable_pic_port(uint16_t);
extern void disable_pic_port(uint16_t);
extern void send_done_pic_master(void);
extern void send_done_pic_slave(void);
extern void interrupt_return(void);
extern void fork_return(Process* p);
extern void exception_page_fault(Interrupt_frame*);



#endif
