/**
 * @file include/kernel.h
 * @brief kernel header
 * @author mopp
 * @version 0.1
 * @date 2014-07-15
 */


#ifndef _KERNEL_H_
#define _KERNEL_H_



#include <stdint.h>


union segment_descriptor;
struct task_state_segment;
struct keyboard;
struct mouse;
struct buddy_manager;
struct tlsf_manager;
struct page_directory_entry;
typedef struct page_directory_entry* Page_directory_table;


/*
 * Axel operating system information structure.
 * This contains important global variable.
 */
struct axel_struct {
    union segment_descriptor* gdt;
    struct task_state_segment* tss;
    struct keyboard* keyboard;
    struct mouse* mouse;
    struct buddy_manager* bman;
    struct tlsf_manager* tman;
    Page_directory_table kernel_pdt;
};
typedef struct axel_struct Axel_struct;


extern Axel_struct axel_s;
extern uintptr_t kernel_init_stack_top;



#endif
