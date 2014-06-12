/************************************************************
 * File: src/interrupt_handler.c
 * Description: Interrupt Handlers
 ************************************************************/

#include <stdint.h>
#include <interrupt.h>



void interrupt_timer(uint32_t* esp) {
    send_done_interrupt_master();
}
