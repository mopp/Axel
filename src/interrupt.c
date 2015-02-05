/**
 * @file interrupt.c
 * @brief Interrupt descriptor and its table.
 *        Setting IDT and interrupt handlers in this.
 * @author mopp
 * @version 0.1
 * @date 2014-07-22
 */
/*
 * | Exception and Interruption in protect mode.
 * | INT #     | Type       | Description
 * ------------+------------+-----------------------------------
 * | 0x00      | fault      | Divide by 0
 * | 0x01      | fault/abort| Reserved
 * | 0x02      | interrupt  | NMI Interrupt
 * | 0x03      | trap       | Breakpoint (INT3)
 * | 0x04      | trap       | Overflow (INTO)
 * | 0x05      | fault      | Bounds range exceeded (BOUND)
 * | 0x06      | fault      | Invalid opcode (UD2)
 * | 0x07      | fault      | Device not available (WAIT/FWAIT)
 * | 0x08      | abort      | Double fault
 * | 0x09      | fault      | Coprocessor segment overrun
 * | 0x0A      | fault      | Invalid TSS
 * | 0x0B      | fault      | Segment not present
 * | 0x0C      | fault      | Stack-segment fault
 * | 0x0D      | fault      | General protection fault
 * | 0x0E      | fault      | Page fault
 * | 0x0F      | fault      | Reserved
 * | 0x10      | fault      | x87 FPU error
 * | 0x11      | fault      | Alignment check
 * | 0x12      | abort      | Machine check
 * | 0x13      | fault      | SIMD Floating-Point Exception
 * | 0x14-0x1F |            | Reserved
 * | 0x20-0xFF | interrupt  | User definable
 * ------------+-----------------------------------
 *
 *  The kind of exception.
 *      "fault"
 *          It is normally recoverable.
 *          And fault is recovered. then program can restart.
 *      "trap"
 *          It is reported after cause of instruction is executed.
 *          And program can run continuous.
 *      "abort"
 *          In abort, cause of instruction is not needed report.
 *          And program that involves abort is cannot run continuous.
 */


#include <stdint.h>
#include <interrupt.h>
#include <macros.h>
#include <asm_functions.h>
#include <segment.h>
#include <utils.h>


enum {
    PIC0_CMD_STATE_PORT  = 0x20,
    PIC0_IMR_DATA_PORT   = 0x21,
    PIC1_CMD_STATE_PORT  = 0xA0,
    PIC1_IMR_DATA_PORT   = 0xA1,

    /* Initialization Control Words */
    PIC0_ICW1            = 0x11, /* Enable ICW4 */
    PIC0_ICW2            = 0x20, /* Interrupt vector base number from PIC. x86 only use bit 3 - 7 */
    PIC0_ICW3            = 0x04, /* Pin number that connected slave PIC. */
    PIC0_ICW4            = 0x01, /* MCS-80/86 mode */
    PIC1_ICW1            = PIC0_ICW1,
    PIC1_ICW2            = 0x28,
    PIC1_ICW3            = 0x02,
    PIC1_ICW4            = PIC0_ICW4,

    /* Operation Command Words2 */
    PIC_OCW2_EOI         = 0x20,
};


Gate_descriptor idt[IDT_MAX_NR];

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

    /* Enable only IRQ02 because it connects slave. */
    enable_pic_port(PIC_IMR_MASK_IRQ02);
}


Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, uintptr_t offset, uint16_t selector_index, uint32_t flags) {
    g->bit_expr_high = flags;
    g->offset_low    = ECAST_UINT16(offset);
    g->offset_high   = ECAST_UINT16(offset >> 16);
    g->seg_selector  = ECAST_UINT16(selector_index * sizeof(Gate_descriptor));
    return g;
}


void init_idt(Set_idt_func f, uintptr_t default_function_addr) {
    /* All idt is clear. */
    for (size_t i = 0; i < IDT_MAX_NR; i++) {
        set_gate_descriptor(idt + i, default_function_addr, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    }

    f(idt, IDT_MAX_NR);

    load_idtr(IDT_LIMIT, (uint32_t)idt);

    init_pic();
}


void enable_pic_port(uint16_t irq_num) {
    if (irq_num < PIC_IMR_MASK_IRQ08) {
        io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) & ~ECAST_UINT8(irq_num));
    } else {
        io_out8(PIC1_IMR_DATA_PORT, io_in8(PIC1_IMR_DATA_PORT) & ~ECAST_UINT8(irq_num >> 8));
    }
}


void disable_pic_port(uint16_t irq_num) {
    if (irq_num < PIC_IMR_MASK_IRQ08) {
        io_out8(PIC0_IMR_DATA_PORT, io_in8(PIC0_IMR_DATA_PORT) | ECAST_UINT8(irq_num));
    } else {
        io_out8(PIC1_IMR_DATA_PORT, io_in8(PIC1_IMR_DATA_PORT) | ECAST_UINT8(irq_num >> 8));
    }
}


void send_done_pic_master(void) {
    io_out8(PIC0_CMD_STATE_PORT, PIC_OCW2_EOI);
}


void send_done_pic_slave(void) {
    io_out8(PIC1_CMD_STATE_PORT, PIC_OCW2_EOI);
}
