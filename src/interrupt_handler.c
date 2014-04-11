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


void interrupt_handler0x20(uint32_t* esp){
    static uint16_t t = 0xff;
    /* static RGB8 rgb = {1, 1, 1, 1}; */
    /* rgb.r = 0x10; */
    /* rgb.g = 0x20; */
    /* rgb.b = 0x00; */
    /* static Point2d p = {0, 0}; */

    ++t;
    if (t % 100 == 0) {
        /* add_point2d(&p, 1, 1); */
        /* fill_rectangle(&make_point2d(0, 0), &p, &rgb); */
    }

    send_done_interrupt_master();
}


void interrupt_handler0x21(uint32_t* esp){
    static Point2d p = {0, 0};
    put_ascii_font("Get Keyboard Interrupt", &p);

    send_done_interrupt_master();
}
