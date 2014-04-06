/************************************************************
 * File: src/interrupt_handler.c
 * Description: Interrupt Handlers
 ************************************************************/

#include <stdint.h>
#include <kernel.h>
#include <graphic.h>
#include <asm_functions.h>
#include <stdio.h>


static inline void send_done_interrupt_master(void) {
    io_out8(PIC0_CMD_STATE_PORT, PIC_OCW2_EOI);
}


static inline void send_done_interrupt_slave(void) {
    io_out8(PIC1_CMD_STATE_PORT, PIC_OCW2_EOI);
}


void interrupt_handler0x20(uint32_t* esp) {
    /* printf("Call interrupt_handler0x20\n"); */
    /* printf("esp is %x\n", esp); */
    uint8_t cnt = 0;
    static RGB8 rgb = {1, 1, 1, 1};
    rgb.r = 0x10;
    rgb.g = 0x20;
    rgb.b = 0x00;

    fill_rectangle(&make_point2d(0, 0), &make_point2d(cnt, cnt), &rgb);

    cnt += 1;

    send_done_interrupt_master();
}
