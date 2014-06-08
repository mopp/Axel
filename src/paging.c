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
static Page_directory_table kernel_pdt;

static size_t get_pde_index(uintptr_t const);
static size_t get_pte_index(uintptr_t const);
static Page_directory_entry* get_pde(Page_directory_table const* const, uintptr_t const);
static Page_table get_pt(Page_directory_entry const* const);
static Page_table_entry* get_pte(Page_table const, uintptr_t const);
static Page_table_entry* set_frame_addr(Page_table_entry* const, uintptr_t const);
static void map_page(Page_directory_table const* const, uint32_t const, uint32_t const, uintptr_t, uintptr_t);
static void map_page_area(Page_directory_table const* const, uint32_t const, uint32_t const, uintptr_t const, uintptr_t const, uintptr_t const, uintptr_t const);
static void map_page_same_area(Page_directory_table const* const, uint32_t const, uint32_t const, uintptr_t const, uintptr_t const);


/* virtual memory allocate */
/* vmalloc */
/* virtual continuous memory allocate */
/* vcmalloc */

void init_paging(Page_directory_table pdt) {
    kernel_pdt = pdt;

    /* init kernel page directory table. */
    memset(pdt, 0, ALL_PAGE_STRUCT_SIZE); /* clear got page directory table area to use. */

    /* calculate and set page table addr to page directory entry */
    uintptr_t pt_base_addr = (vir_to_phys_addr((uintptr_t)pdt + (sizeof(Page_directory_entry) * (PAGE_DIRECTORY_ENTRY_NUM)))) >> PDE_PT_ADDR_SHIFT_NUM;
    for (size_t i = 0; i < PAGE_DIRECTORY_ENTRY_NUM; i++) {
        kernel_pdt[i].page_table_addr = (pt_base_addr + i) & 0xFFFFF;
    }

    /* set kernel area paging and video area paging. */
    map_page_area(&kernel_pdt, PDE_FLAGS_FOR_KERNEL, PTE_FLAGS_FOR_KERNEL, get_kernel_vir_start_addr(), get_kernel_vir_end_addr(), get_kernel_phys_start_addr(), get_kernel_phys_end_addr());
    map_page_same_area(&kernel_pdt, PDE_FLAGS_FOR_KERNEL, PTE_FLAGS_FOR_KERNEL, 0xfd000000, 0xfd000000 + (600 * 800 * 4));

    /* switch paging directory table. */
    set_cpu_pdt((uintptr_t)kernel_pdt);
    turn_off_4MB_paging();
}


static inline size_t get_pde_index(uintptr_t const addr) {
    return (addr >> PDE_IDX_SHIFT_NUM) & 0x03FF;
}


static inline size_t get_pte_index(uintptr_t const addr) {
    return (addr >> PTE_IDX_SHIFT_NUM) & 0x03FF;
}


static inline Page_directory_entry* get_pde(Page_directory_table const* const pdt, uintptr_t const vaddr) {
    return *pdt + get_pde_index(vaddr);
}


static inline Page_table get_pt(Page_directory_entry const* const pde) {
    return (Page_table)(phys_to_vir_addr((uintptr_t)(pde->page_table_addr << PDE_PT_ADDR_SHIFT_NUM)));
}


static inline Page_table_entry* get_pte(Page_table const pt, uintptr_t const vaddr) {
    return pt + get_pte_index(vaddr);
}


static inline Page_table_entry* set_frame_addr(Page_table_entry* const pte, uintptr_t const paddr) {
    pte->frame_addr = (paddr >> PTE_FRAME_ADDR_SHIFT_NUM) & 0xFFFFF;
    return pte;
}


/**
 * @brief Mapping page at virtual addr to physical addr.
 *          And overwrite pde and pte flags.
 * @param pdt       pointer to Page_directory_table.
 * @param pde_flags flags for Page_directory_entry.
 * @param pte_flags flags for Page_table_entry.
 * @param vaddr     virtual address.
 * @param paddr     physical address.
 */
static inline void map_page(Page_directory_table const* const pdt, uint32_t const pde_flags, uint32_t const pte_flags, uintptr_t vaddr, uintptr_t paddr) {
    Page_directory_entry* const pde = get_pde(pdt, vaddr);
    pde->bit_expr = (PDE_FLAGS_AREA_MASK & pde->bit_expr) | pde_flags;

    Page_table_entry* const pte = get_pte(get_pt(pde), vaddr);
    pte->bit_expr = (PTE_FLAGS_AREA_MASK & pte->bit_expr) | pte_flags;

    set_frame_addr(pte, paddr);
}


/**
 * @brief Mapping page area from begin_vaddr ~ end_vaddr to begin_paddr ~ end_paddr.
 *          And overwrite pde and pte flags.
 * @param pdt           Pointer to Page_directory_table.
 * @param pde_flags     Flags for Page_directory_entry.
 * @param pte_flags     Flags for Page_table_entry.
 * @param begin_vaddr   Begin virtual address of mapping area.
 * @param end_vaddr     End virtual address of mapping area.
 * @param begin_paddr   Begin physical address of mapping area.
 * @param end_paddr     End physical address of mapping area.
 */
static inline void map_page_area(Page_directory_table const* const pdt, uint32_t const pde_flags, uint32_t const pte_flags, uintptr_t const begin_vaddr, uintptr_t const end_vaddr, uintptr_t const begin_paddr, uintptr_t const end_paddr) {
    /* Generally speaking, PAGE_SIZE equals FRAME_SIZE. */
    for (uintptr_t vaddr = begin_vaddr, paddr = begin_paddr; (vaddr < end_vaddr) && (paddr < end_paddr); vaddr += PAGE_SIZE, paddr += FRAME_SIZE) {
        map_page(pdt, pde_flags, pte_flags, vaddr, paddr);
    }
}


/**
 * @brief Mapping page area same physical address to virtual address.
 *          And overwrite pde and pte flags.
 * @param pdt           Pointer to Page_directory_table.
 * @param pde_flags     Flags for Page_directory_entry.
 * @param pte_flags     Flags for Page_table_entry.
 * @param begin_addr    Begin address of mapping area.
 * @param end_addr      End address of mapping area.
 */
static inline void map_page_same_area(Page_directory_table const* const pdt, uint32_t const pde_flags, uint32_t const pte_flags, uintptr_t const begin_addr, uintptr_t const end_addr) {
    map_page_area(pdt, pde_flags, pte_flags, begin_addr, end_addr, begin_addr, end_addr);
}
