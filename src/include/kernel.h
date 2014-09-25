/**
 * @file include/kernel.h
 * @brief kernel header
 * @author mopp
 * @version 0.1
 * @date 2014-07-15
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_



#include <ps2.h>
#include <segment.h>
#include <buddy.h>


/*
 * Axel operating system information structure.
 * This contains important global variable.
 */
struct axel_struct {
    Segment_descriptor* gdt;
    Task_state_segment* tss;
    Keyboard* keyboard;
    Mouse* mouse;
    Buddy_manager* bman;
};
typedef struct axel_struct Axel_struct;


/* FIXME */
extern Axel_struct axel_s;
extern uintptr_t kernel_init_stack_top;



#endif
