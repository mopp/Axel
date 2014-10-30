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
#include <paging.h>


/*
 * "Task"/"Interrupt"/"Trap" gate descriptor
 * Axel only use "Interrupt" and "Trap" gate descriptor.
 * The difference between Interrupt and Trap gate descriptor is below.
 * In interrupt gate, "CPU clears IF flag(interrupt enable flag)" in eflags register to avoid occurrence another interrupt.
 * And CPU restore eflags register using stored eflags register valus in stack.
 * In trap gate, CPU does not clear IF flag.
 */
union gate_descriptor {
    struct {
        uint16_t offset_low;            /* address offset low of interrupt handler.*/
        uint16_t seg_selector;          /* segment selector register(CS, DS, SS and etc) value. */
        uint8_t unused_zero;            /* unused area. */
        unsigned int type : 3;          /* gate type. */
        unsigned int size : 1;          /* If this is 1, gate size is 32 bit. otherwise it is 16 bit */
        unsigned int zero_reserved : 1; /* reserved area. */
        unsigned int plivilege : 2;     /* This controles accesse level. */
        unsigned int is_present : 1;    /* Is it exist on memory */
        uint16_t offset_high;           /* address offset high of interrupt handler.*/
    };
    struct {
        uint32_t bit_expr_low;
        uint32_t bit_expr_high;
    };
};
typedef union gate_descriptor Gate_descriptor;
_Static_assert(sizeof(Gate_descriptor) == 8, "Gate_descriptor size is NOT 8 byte.");


enum {
    IDT_MAX_NR        = 256,
    IDT_LIMIT         = IDT_MAX_NR * sizeof(Gate_descriptor) - 1,
    GD_TYPE_TASK      = 0x00000500,
    GD_TYPE_INTERRUPT = 0x00000600,
    GD_TYPE_TRAP      = 0x00000700,
    GD_SIZE_32        = 0x00000800,
    GD_RING0          = 0x00000000,
    GD_RING1          = 0x00002000,
    GD_RING2          = 0x00004000,
    GD_RING3          = 0x00006000,
    GD_PRESENT        = 0x00008000,
    GD_FLAGS_INT      = GD_TYPE_INTERRUPT | GD_SIZE_32 | GD_RING0 | GD_PRESENT,
    GD_FLAGS_TRAP     = GD_TYPE_TRAP | GD_SIZE_32 | GD_RING0 | GD_PRESENT,
};


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


static Gate_descriptor idt[IDT_MAX_NR];

extern void asm_exception_page_fault(void);
extern void asm_interrupt_timer(void);
extern void asm_interrupt_keybord(void);
extern void asm_interrupt_mouse(void);
extern void asm_syscall_enter(void);


static void hlt(Interrupt_frame* ic) {
    DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, ic);
}


static void no_op(Interrupt_frame* ic) {
    puts("No Operation");
    printf("eip: 0x%x\n", ic->eip);
    /* TODO: kill caller process. */
}


static inline void init_pic(void) {
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


static inline Gate_descriptor* set_gate_descriptor(Gate_descriptor* g, void* offset, uint16_t selector_index, uint32_t flags) {
    g->bit_expr_high = flags;
    g->offset_low    = ECAST_UINT16((uintptr_t)offset);
    g->offset_high   = ECAST_UINT16((uintptr_t)offset >> 16);
    g->seg_selector  = ECAST_UINT16(selector_index * sizeof(Gate_descriptor));
    return g;
}


void init_idt(void) {
    /* All idt is clear. */
    for (size_t i = 0; i < IDT_MAX_NR; i++) {
        set_gate_descriptor(idt + i, no_op, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    }

    set_gate_descriptor(idt + 0x08, hlt,                      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    set_gate_descriptor(idt + 0x0D, hlt,                      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    set_gate_descriptor(idt + 0x0E, asm_exception_page_fault, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_TRAP);
    set_gate_descriptor(idt + 0x20, asm_interrupt_timer,      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    set_gate_descriptor(idt + 0x21, asm_interrupt_keybord,    KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    set_gate_descriptor(idt + 0x2C, asm_interrupt_mouse,      KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
    set_gate_descriptor(idt + 0x80, asm_syscall_enter,        KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_TRAP | GD_RING3);

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


void exception_page_fault(Interrupt_frame* ic) {
    uintptr_t fault_addr = load_cr2();
    io_cli();
    BOCHS_MAGIC_BREAK();

    if (fault_addr < KERNEL_VIRTUAL_BASE_ADDR) {
        /* TODO: User space fault */
        io_cli();
        printf("\nfault_addr: 0x%zx\n", fault_addr);
        puts("User space fault or Kernel space fault");
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x1);
    }

    /* Kernel space fault */
    if (is_kernel_pdt((Page_directory_table)(phys_to_vir_addr(get_cpu_pdt()))) == true) {
        /* Maybe kernel error */
        /* TODO: panic */
        io_cli();
        printf("\nfault_addr: 0x%zx\n", fault_addr);
        puts("Kernel space fault");
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x2);
    }

    Axel_state_code result = synchronize_pdt(fault_addr);
    if (result != AXEL_SUCCESS) {
        /* TODO: panic */
        io_cli();
        printf("\nfault_addr: 0x%zx\n", fault_addr);
        puts("Synchronize fault");
        DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, 0x3);
    }
}
