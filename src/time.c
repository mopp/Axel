/**
 * @file time.c
 * @brief Time implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-30
 */

#include <interrupt.h>
#include <asm_functions.h>
#include <proc.h>
#include <time.h>


static uint32_t tick_count = 0;


void init_pit(void) {
    io_out8(PIT_PORT_CONTROL, PIT_ICW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_LOW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_HIGH);

    enable_pic_port(PIC_IMR_MASK_IRQ00);
}


void interrupt_timer(Interrupt_frame* ic) {
    /* 1 tick is 10ms */
    ++tick_count;

    send_done_pic_master();
    if (is_enable_process == true) {
        switch_context(ic);
    }
}


void wait(uint32_t ms) {
    uint32_t end = tick_count + ((ms / 10) + (ms % 10 != 0 ? 1 : 0));

    while (tick_count <= end)
        ;
}
