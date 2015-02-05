/**
 * @file include/interrupt.h
 * @brief Interrupt handler header
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */


#ifndef _INTERRUPT_HANDLER_H_
#define _INTERRUPT_HANDLER_H_



#include <stdint.h>


/* Programmable Interrupt Controller */
enum {
    PIC_IMR_MASK_IRQ00   = 0x0001,
    PIC_IMR_MASK_IRQ01   = 0x0002,
    PIC_IMR_MASK_IRQ02   = 0x0004,
    PIC_IMR_MASK_IRQ03   = 0x0008,
    PIC_IMR_MASK_IRQ04   = 0x0010,
    PIC_IMR_MASK_IRQ05   = 0x0020,
    PIC_IMR_MASK_IRQ06   = 0x0040,
    PIC_IMR_MASK_IRQ07   = 0x0080,
    PIC_IMR_MASK_IRQ08   = 0x0100,
    PIC_IMR_MASK_IRQ09   = 0x0200,
    PIC_IMR_MASK_IRQ10   = 0x0400,
    PIC_IMR_MASK_IRQ11   = 0x0800,
    PIC_IMR_MASK_IRQ12   = 0x1000,
    PIC_IMR_MASK_IRQ13   = 0x2000,
    PIC_IMR_MASK_IRQ14   = 0x4000,
    PIC_IMR_MASK_IRQ15   = 0x8000,
    PIC_IMR_MASK_IRQ_ALL = 0xFF,
};


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


/*
 * This is stack structure when happen interruption by CPU.
 * Please see ./interrupt_asm.asm
 */
struct interrupt_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint16_t gs, reserved1;
    uint16_t fs, reserved2;
    uint16_t es, reserved3;
    uint16_t ds, reserved4;
    uint32_t error_code;
    uint32_t eip;
    uint16_t cs, reserved5;
    uint32_t eflags;
    uint32_t prev_esp;
    uint16_t prev_ss, reserved6;
};
typedef struct interrupt_frame Interrupt_frame;


struct process;
typedef struct process Process;


typedef void (*Set_idt_func)(Gate_descriptor*, size_t);


extern void init_idt(Set_idt_func, uintptr_t);
extern void init_pic(void);
extern Gate_descriptor* set_gate_descriptor(Gate_descriptor*, uintptr_t, uint16_t, uint32_t);
extern void enable_pic_port(uint16_t);
extern void disable_pic_port(uint16_t);
extern void send_done_pic_master(void);
extern void send_done_pic_slave(void);
extern void interrupt_return(void);
extern void fork_return(Process* p);
extern void asm_exception_page_fault(void);
extern void asm_interrupt_timer(void);
extern void asm_interrupt_keybord(void);
extern void asm_interrupt_mouse(void);
extern void asm_syscall_enter(void);



#endif
