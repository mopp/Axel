/************************************************************
 * File: include/interrupt.h
 * Description: Interrupt Handler Header
 ************************************************************/

#ifndef _INTERRUPT_HANDLER_H_
#define _INTERRUPT_HANDLER_H_



#include <kernel.h>
#include <asm_functions.h>


static inline void send_done_interrupt_master(void) {
    io_out8(PIC0_CMD_STATE_PORT, PIC_OCW2_EOI);
}


static inline void send_done_interrupt_slave(void) {
    io_out8(PIC1_CMD_STATE_PORT, PIC_OCW2_EOI);
}



extern void asm_interrupt_timer(void);
extern void asm_interrupt_keybord(void);



#endif
