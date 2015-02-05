/**
 * @file loader2.c
 * @brief Second OS boot loader
 * @author mopp
 * @version 0.1
 * @date 2015-02-03
 */

#include <stdint.h>
#include <stddef.h>
#include <interrupt.h>
#include <asm_functions.h>
#include <segment.h>
#include <time.h>
#include <macros.h>


enum {
    LOADER2_ADDR = 0x7C00,
};


struct memory_info {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attr;
};
typedef struct memory_info_e820 Memory_info;


static _Noreturn void no_op(void);
static void set_idt(Gate_descriptor*, size_t);
static void clear_bss(void);
static void fill_screen(uint8_t);


_Noreturn void loader2(Memory_info* infos, uint32_t size, uint32_t loader2_size, uint16_t drive_number) {
    io_cli();
    __asm__ volatile(
            "mov $0x10, %ax \n"
            "mov %ax, %fs\n"
            "mov %ax, %gs\n"
            );
    clear_bss();

    BOCHS_MAGIC_BREAK();
    init_idt(set_idt, (uintptr_t)no_op);
#if 1
    /* init_pit */
    io_out8(PIT_PORT_CONTROL, PIT_ICW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_LOW);
    io_out8(PIT_PORT_COUNTER0, PIT_COUNTER_VALUE_HIGH);
    enable_pic_port(PIC_IMR_MASK_IRQ00);
    /* io_sti(); */
#endif

    fill_screen(0x0c);
    for (;;) {
        __asm__ volatile(
                "movl %%eax, %%edi\n"
                "hlt\n"
                :
                : "a"(loader2_size)
                :
                );
    }
}


static uint32_t tick_count = 0;
static void timer(void) {
    /* 1 tick is 10ms */
    ++tick_count;
    fill_screen(tick_count & 0xf);
}


static void set_idt(Gate_descriptor* idts, size_t size) {
    set_gate_descriptor(idts + 0x20, (uintptr_t)timer, KERNEL_CODE_SEGMENT_INDEX, GD_FLAGS_INT);
}


static _Noreturn void no_op(void) {
    uint8_t color = 1;
    for (;;) {
        fill_screen(color++);
    }
}


static inline void clear_bss(void) {
    extern uintptr_t const LD_LOADER2_BSS_BEGIN;
    extern uintptr_t const LD_LOADER2_BSS_END;

    for (uintptr_t i = LD_LOADER2_BSS_BEGIN; i < LD_LOADER2_BSS_END; i++) {
        *((uint8_t*)i) = 0;
    }
}


static inline void fill_screen(uint8_t color) {
    for (uint8_t* v = (uint8_t*)0xA0000; (uintptr_t)v < 0xB0000; v++) {
        *v = color;
    }
}
