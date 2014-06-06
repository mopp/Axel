/**
 * @file include/memory.h
 * @brief physical memory initialize and manage.
 * @author mopp
 * @version 0.1
 * @date 2014-06-05
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_



#include <stdint.h>
#include <stddef.h>
#include <multiboot.h>


extern void init_memory(Multiboot_memory_map const*, size_t const);
extern void* pmalloc(size_t);
extern void* pmalloc_page_round(size_t);
extern void pfree(void*);
extern void print_mem(void);
extern uintptr_t get_kernel_vir_start_addr(void);
extern uintptr_t get_kernel_vir_end_addr(void);
extern uintptr_t get_kernel_phys_start_addr(void);
extern uintptr_t get_kernel_phys_end_addr(void);
extern size_t get_kernel_size(void);
extern size_t get_kernel_static_size(void);



#endif
