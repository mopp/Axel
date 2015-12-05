/**
 * @file time.c
 * @brief Time implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-30
 */

#include <interrupt.h>
#include <asm_functions.h>
#include <time.h>
#include <utils.h>


enum {
    MAX_TIMER_HANDLER_NUM = 16,
};

static uint32_t tick_count = 0;
static size_t handler_count = 0;
static Timer_handler* timer_handlers[MAX_TIMER_HANDLER_NUM];


void init_pit(void) {
    io_out8(PIT_PORT_CONTROL, PIT_ICW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_LOW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_HIGH);

    enable_pic_port(PIC_IMR_MASK_IRQ00);

    memset(timer_handlers, (int)NULL, sizeof(Timer_handler) * MAX_TIMER_HANDLER_NUM);
}


Axel_state_code timer_handler_add(Timer_handler* handler) {
    if (MAX_TIMER_HANDLER_NUM == handler_count) {
        return AXEL_FAILED;
    }

    handler_count++;
    for (size_t i = 0; i < MAX_TIMER_HANDLER_NUM; i++) {
        if (timer_handlers[i] == NULL) {
            timer_handlers[i] = handler;
            handler->id = i;
            break;
        }
    }

    return AXEL_SUCCESS;
}


Axel_state_code timer_handler_remove(Timer_handler* handler) {
    if (memcmp(timer_handlers[handler->id], handler, sizeof(Timer_handler)) != 0) {
        return AXEL_FAILED;
    }
    timer_handlers[handler->id] = NULL;

    return AXEL_SUCCESS;
}


void interrupt_timer(Interrupt_frame* ic) {
    /* 1 tick is 10ms */
    ++tick_count;

    /* NOTE: this function call is need at here for system call. */
    send_done_pic_master();

    /* Execute handler. */
    for (size_t i = 0; i < MAX_TIMER_HANDLER_NUM; i++) {
        Timer_handler* th = timer_handlers[i];
        if (th != NULL) {
            th->on_tick(ic, th);
        }
    }

}


void wait(uint32_t ms) {
    uint32_t end = tick_count + ((ms / 10) + (ms % 10 != 0 ? 1 : 0));

    while (tick_count <= end)
        ;
}
