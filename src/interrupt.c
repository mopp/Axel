/************************************************************
 * File: src/interrupt_handler.c
 * Description: Interrupt Handlers
 ************************************************************/

#include <stdint.h>
#include <interrupt.h>
#include <macros.h>
#include <asm_functions.h>
#include <proc.h>


void init_pic(void) {
    /* Master */
    io_out8(PIC0_CMD_STATE_PORT, PIC0_ICW1);
    io_out8(PIC0_IMR_DATA_PORT, PIC0_ICW2);
    io_out8(PIC0_IMR_DATA_PORT, PIC0_ICW3);
    io_out8(PIC0_IMR_DATA_PORT, PIC0_ICW4);

    /* Slave */
    io_out8(PIC1_CMD_STATE_PORT, PIC1_ICW1);
    io_out8(PIC1_IMR_DATA_PORT, PIC1_ICW2);
    io_out8(PIC1_IMR_DATA_PORT, PIC1_ICW3);
    io_out8(PIC1_IMR_DATA_PORT, PIC1_ICW4);

    /* Disable all interrupt. */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);
    io_out8(PIC1_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);

    /* enable only IRQ02 because IRQ02 connects slave. */
    enable_interrupt(PIC_IMR_MASK_IRQ02);
}


void enable_interrupt(uint16_t irq_num) {
    if (irq_num < PIC_IMR_MASK_IRQ08) {
        io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) & ~ECAST_UINT8(irq_num));
    } else {
        io_out8(PIC1_IMR_DATA_PORT, io_in8(PIC1_IMR_DATA_PORT) & ~ECAST_UINT8(irq_num >> 8));
    }
}


void disable_interrupt(uint16_t irq_num) {
    if (irq_num < PIC_IMR_MASK_IRQ08) {
        io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) | ECAST_UINT8(irq_num));
    } else {
        io_out8(PIC1_IMR_DATA_PORT, io_in8(PIC1_IMR_DATA_PORT) | ECAST_UINT8(irq_num >> 8));
    }
}


void send_done_interrupt_master(void) {
    io_out8(PIC0_CMD_STATE_PORT, PIC_OCW2_EOI);
}


void send_done_interrupt_slave(void) {
    io_out8(PIC1_CMD_STATE_PORT, PIC_OCW2_EOI);
}


void interrupt_timer(Interrupt_frame* ic) {
    send_done_interrupt_master();
    if (is_enable_process == true) {
        switch_context();
    }
}
