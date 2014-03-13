/************************************************************
 * File: src/interrupt_handler.c
 * Description: Interrupt Handlers
 ************************************************************/

#include <stdint.h>
#include <kernel.h>
#include <asm_functions.h>
#include <graphic.h>


void interrupt_handler0x20(uint32_t* esp) {
    io_out8(PIC0_CMD_STATE_PORT, PIC_OCW2_EOI);

    printf("Call interrupt_handler0x20\n");
    printf("esp is %x\n", esp);
}
