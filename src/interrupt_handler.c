/************************************************************
 * File: src/interrupt_handler.c
 * Description: Interrupt Handlers
 ************************************************************/

#include <stdint.h>
#include <kernel.h>
#include <asm_functions.h>
#include <graphic_txt.h>

void interrupt_handler0x20(uint32_t* esp) {

    io_out8(PIC0_CMD_STATE_PORT, PIC0_OCW2);
    printf("Call interrupt_handler0x20\n");
}
