/************************************************************
 * File: src/interrupt_handler.c
 * Description: Interrupt Handlers
 ************************************************************/

#include <stdint.h>
#include <interrupt.h>


void init_pic(void) {
    /* 全割り込みを受けない */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);
    io_out8(PIC1_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL);

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

    /* IRQ2以外の割り込みを受け付けない */
    io_out8(PIC0_IMR_DATA_PORT, PIC_IMR_MASK_IRQ_ALL & (~PIC_IMR_MASK_IRQ2));
}


void interrupt_timer(uint32_t* esp) {
    send_done_interrupt_master();
}
