/**
 * @file paging.c
 * @brief paging implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-05-20
 */
#include <paging.h>
#include <memory.h>
#include <asm_functions.h>

/*
 * kernel page directory table.
 * This contains page that is assigned KERNEL_VIRTUAL_BASE_ADDR to KERNEL_VIRTUAL_BASE_ADDR + get_kernel_size().
 */
static Page_directory_table kernel_pdt;


static inline size_t get_pde_index(uintptr_t const);
static inline size_t get_pte_index(uintptr_t const);


void init_paging(void) {
    /* Re make kernel paging. */

    size_t const size = get_kernel_size();
    size_t kp_num = (size / FRAME_SIZE) + ((size % FRAME_SIZE != 0) ? 1 : 0); /* kernel needs to kp_num pages. */


    /* set_cpu_pdt((uintptr_t)kernel_pdt); */
}


static inline Page_directory_entry* get_pde(Page_directory_table const* const pdt, uintptr_t vaddr) {
    return &(*pdt)[get_pde_index(vaddr)];
}


static inline Page_table* get_pt(Page_directory_entry const* const pde) {
    return (Page_table*)(uintptr_t)(pde->page_table_addr << 12);
}


static inline Page_table_entry* get_pte(Page_table const* const pt, uintptr_t vaddr) {
    return &(*pt)[get_pte_index(vaddr)];
}


static inline Page_table_entry* set_frame_addr(Page_table_entry * const pte, uintptr_t paddr) {
    pte->frame_addr = (paddr >> 12) & 0xFFFFF;
    return pte;
}


/**
 * @brief mapping page at virtual addr to physical addr.
 * @param vaddr virtual address.
 * @param paddr physical address.
 */
static inline void map_kernel_page(uintptr_t vaddr, uintptr_t paddr) {
    Page_directory_entry pde = *get_pde(&kernel_pdt, vaddr);
    Page_table pt = *get_pt(&pde);
    Page_table_entry pte = *get_pte(&pt, vaddr);
    set_frame_addr(&pte, paddr);
}


static inline size_t get_pde_index(uintptr_t const addr) {
    return (addr >> 22) & 0x03FF;
}


static inline size_t get_pte_index(uintptr_t const addr) {
    return (addr >> 12) & 0x03FF;
}
