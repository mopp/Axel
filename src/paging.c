/**
 * @file paging.c
 * @brief paging implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-05-20
 */
#include <paging.h>


inline uintptr_t phys_to_vir_addr(uintptr_t addr) {
    return addr + KERNEL_VIRTUAL_BASE_ADDR;
}


inline uintptr_t vir_to_phys_addr(uintptr_t addr) {
    return addr - KERNEL_VIRTUAL_BASE_ADDR;
}
