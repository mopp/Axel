/**
 * @file paging.c
 * @brief paging implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-05-20
 */
#include <paging.h>
#include <string.h>
#include <memory.h>
#include <asm_functions.h>

/*
 * kernel page directory table.
 * This contains page that is assigned KERNEL_VIRTUAL_BASE_ADDR to KERNEL_VIRTUAL_BASE_ADDR + get_kernel_size().
 */
static Page_directory_entry* kernel_pdt;


// static size_t get_pde_index(uintptr_t const);
// static size_t get_pte_index(uintptr_t const);
// static Page_directory_entry* get_pde(Page_directory_table const* const, uintptr_t);
// static Page_table get_pt(Page_directory_entry const* const);
// static Page_table_entry* get_pte(Page_table, uintptr_t);
// static Page_table_entry* set_frame_addr(Page_table_entry* const, uintptr_t);
// static Page_table_entry* get_pte_by_addr(Page_directory_table const* const, uintptr_t);
// static void map_kernel_page(uintptr_t, uintptr_t);


static size_t get_pde_index(uintptr_t const addr) {
    return (addr >> 22) & 0x03FF;
}


static size_t get_pte_index(uintptr_t const addr) {
    return (addr >> 12) & 0x03FF;
}


static Page_directory_entry* get_pde(Page_directory_entry* pdt, uintptr_t vaddr) {
    return pdt + get_pde_index(vaddr);
}


static Page_table_entry* get_pt(Page_directory_entry** pde) {
    return (Page_table_entry*)(phys_to_vir_addr((uintptr_t)((*pde)->page_table_addr << 12)));
}


static Page_table_entry* get_pte(Page_table_entry* pt, uintptr_t vaddr) {
    return pt + get_pte_index(vaddr);
}


static Page_table_entry* set_frame_addr(Page_table_entry* const pte, uintptr_t paddr) {
    pte->frame_addr = (paddr >> 12) & 0xFFFFF;
    return pte;
}


/* static Page_table_entry get_pte_by_addr(Page_directory_table const* const pdt, uintptr_t vaddr) { */
/* return *get_pte(&get_pt(get_pde(&kernel_pdt, vaddr)), vaddr); */
/* } */


/**
 * @brief mapping page at virtual addr to physical addr.
 * @param vaddr virtual address.
 * @param paddr physical address.
 */
uintptr_t debug;
static void map_kernel_page(uintptr_t vaddr, uintptr_t paddr) {
    Page_directory_entry* pde = get_pde(kernel_pdt, vaddr); /* 0xC0125C00 */
    if (pde->preset_flag == 0) {
        /* init */
        pde->preset_flag = 1;
        pde->read_write_flag = 1;
        pde->global_flag = 1;
    }

    Page_table_entry* pt = get_pt(&pde);        /* 0xC0426000 */
    Page_table_entry* pte = get_pte(pt, vaddr); /* 0xC0426400 */
    debug = (uintptr_t)pte;

    if (pte->preset_flag == 0) {
        /* init */
        pte->preset_flag = 1;
        pte->read_write_flag = 1;
        pte->global_flag = 1;
    }

    set_frame_addr(pte, paddr);
}


/* Re make kernel paging. */
void init_paging(Page_directory_entry* pdt) {
    kernel_pdt = pdt; /* 0xC0125000 */

    /* init kernel page directory table. */
    memset(pdt, ALL_PAGE_STRUCT_SIZE, 0);
    uintptr_t pt_addr = vir_to_phys_addr((uintptr_t)pdt + sizeof(Page_directory_entry) * (PAGE_DIRECTORY_ENTRY_NUM)); /* 0xC0126000 */
    pt_addr >>= 12;

    for (size_t i = 0; i < PAGE_DIRECTORY_ENTRY_NUM; i++) {
        kernel_pdt[i].page_table_addr = 0xFFFFF & (pt_addr + i);
    }

    uintptr_t vir_addr = get_kernel_vir_start_addr();
    uintptr_t phys_addr = get_kernel_phys_start_addr();
    uintptr_t vir_end = get_kernel_vir_end_addr();
    uintptr_t phys_end = get_kernel_phys_end_addr();
    while (vir_addr < vir_end && phys_addr < phys_end) {
        map_kernel_page(vir_addr, phys_addr);

        /* Generally, PAGE_SIZE equals FRAME_SIZE. */
        vir_addr += PAGE_SIZE;
        phys_addr += FRAME_SIZE;
    }

    vir_addr = 0xFD000000;
    phys_addr = 0xFD000000;
    vir_end = vir_addr + (600 * 800 * 4);
    phys_end = vir_addr + (600 * 800 * 4);
    while (vir_addr < vir_end && phys_addr < phys_end) {
        map_kernel_page(vir_addr, phys_addr);

        /* Generally, PAGE_SIZE equals FRAME_SIZE. */
        vir_addr += PAGE_SIZE;
        phys_addr += FRAME_SIZE;
    }

    set_cpu_pdt(vir_to_phys_addr((uintptr_t)kernel_pdt));
}
