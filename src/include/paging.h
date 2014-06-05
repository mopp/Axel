/**
 * @file paging.c
 * @brief paging header.
 *      NOTE that "frame" is also physical page or page frame.
 * @author mopp
 * @version 0.1
 * @date 2014-05-20
 */
#ifndef _PAGING_H
#define _PAGING_H



#define KERNEL_PHYSICAL_BASE_ADDR 0x100000
#define KERNEL_VIRTUAL_BASE_ADDR 0xC0000000



#ifndef _ASSEMBLY



#include <stdint.h>
#include <stddef.h>

/*
 * In x86_64
 * "Two-Level paging"
 *      page_table_entry -> page_table(page_table_entry[1024]) -> page_directory_entry -> page_directory_table(page_directory_entry[1024])
 *      one page_table should have 1024 page.
 *      one page_directory_table should have 1024 page_table.
 *      the page_table size is 1024(PAGE_NUM) * 4KB(PAGE_SIZE) = 4MB
 *      extern Page_table_entry page_table[PAGE_TABLE_SIZE];
 */


/*
 * This structure corresponds with page frame on physical memory
 * And store those data.
 */
struct page_table_entry {
    unsigned int preset_flag : 1;                   /* This is allocated phys memory. */
    unsigned int read_write_flag : 1;               /* Read only for 0 or Writeable and Readable for 1. */
    unsigned int user_supervisor_flag : 1;          /* Page authority. */
    unsigned int page_level_write_throgh_flag : 1;  /* キャッシュ方式が ライトバック(0), ライトスルー(1) */
    unsigned int page_level_cache_disable_flag : 1; /* キャッシュを有効にするか否か */
    unsigned int access_flag : 1;                   /* Is page accessed. */
    unsigned int dirty_flag : 1;                    /* Is page changed. */
    unsigned int page_attr_table_flag : 1;          /* For PAT feature. */
    unsigned int global_flag : 1;                   /* Is page global. */
    unsigned int os_area : 3;                       /* This 3bit is ignored by CPU and OS can use this area. */
    unsigned int frame_addr : 20;                   /* upper 20bit of physical address to assign to page entry. */
};
typedef struct page_table_entry Page_table_entry;
_Static_assert(sizeof(Page_table_entry) == 4, "Static ERROR : Page_table_entry size is NOT 4 byte(32 bit).");


/*
 * Page table have 1024 page table entry.
 */
typedef Page_table_entry* Page_table;


/*
 * This structure corresponds with page directory entry.
 */
struct page_directory_entry {
    unsigned int preset_flag : 1;
    unsigned int read_write_flag : 1;
    unsigned int user_supervisor_flag : 1;
    unsigned int page_level_write_throgh_flag : 1;
    unsigned int page_level_cache_disable_flag : 1;
    unsigned int access_flag : 1;
    unsigned int reserved : 1;
    unsigned int page_size_flag : 1;   /* このPDE内のページのサイズを指定する 4KB(0) or 4MB(1)*/
    unsigned int global_flag : 1;      /* Is page global. */
    unsigned int os_area : 3;          /* This 3bit is ignored by CPU and OS can use this area. */
    unsigned int page_table_addr : 20; /* physical address of page table that is included page directory entry. */
};
typedef struct page_directory_entry Page_directory_entry;
_Static_assert(sizeof(Page_directory_entry) == 4, "Static ERROR : Page_directory_entry size is NOT 4 byte(32 bit).");


/*
 * Page directory table have 1024 page directory entry.
 */
typedef Page_directory_entry* Page_directory_table;


enum Paging_constants {
    FRAME_SIZE = 4096,
    PAGE_SIZE = FRAME_SIZE,
    PAGE_NUM = 1024,
    PAGE_TABLE_ENTRY_NUM = 1024,
    PAGE_DIRECTORY_ENTRY_NUM = 1024,
    PDE_PT_ADDR_SHIFT_NUM = 12,
    PDE_IDX_SHIFT_NUM = 22,
    PTE_IDX_SHIFT_NUM = 12,
    PTE_FRAME_ADDR_SHIFT_NUM = 12,
    ALL_PAGE_STRUCT_SIZE = (sizeof(Page_directory_entry) * PAGE_DIRECTORY_ENTRY_NUM + sizeof(Page_table_entry) * PAGE_TABLE_ENTRY_NUM * PAGE_DIRECTORY_ENTRY_NUM),
};


static inline uintptr_t phys_to_vir_addr(uintptr_t addr) {
    return addr + KERNEL_VIRTUAL_BASE_ADDR;
}


static inline uintptr_t vir_to_phys_addr(uintptr_t addr) {
    return addr - KERNEL_VIRTUAL_BASE_ADDR;
}


static inline void set_phys_to_vir_addr(void* addr) {
    uintptr_t* p = (uintptr_t*)addr;
    *p = phys_to_vir_addr(*p);
}


static inline size_t round_page_size(size_t size) {
    /*
     * Simple expression is below.
     * ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE
     * In particular, PAGE_SIZE is power of 2.
     * We calculate same thing using below code.
     */
    return ((size + PAGE_SIZE - 1) & 0xFFFFFF000);
}


extern void init_paging(Page_directory_table);



#endif /* _ASSEMBLY */



#endif
