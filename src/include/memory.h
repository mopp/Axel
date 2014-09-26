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
#include <state_code.h>


extern Axel_state_code init_memory(Multiboot_info* const);
extern void* pmalloc(size_t);
extern void pfree(void*);
extern uintptr_t get_kernel_vir_start_addr(void);
extern uintptr_t get_kernel_vir_end_addr(void);
extern uintptr_t get_kernel_phys_start_addr(void);
extern uintptr_t get_kernel_phys_end_addr(void);
extern size_t get_kernel_size(void);
extern size_t get_kernel_static_size(void);
extern size_t get_total_memory_size(void);



#endif
