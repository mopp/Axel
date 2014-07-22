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
static Page_manager* p_man;
static List_node* p_list_nodes;
static Page_info* p_info;
static bool* used_list_node;

static size_t get_pde_index(uintptr_t const);
static size_t get_pte_index(uintptr_t const);
static Page_directory_entry* get_pde(Page_directory_table const* const, uintptr_t const);
static Page_table get_pt(Page_directory_entry const* const);
static Page_table_entry* get_pte(Page_table const, uintptr_t const);
static Page_table_entry* set_frame_addr(Page_table_entry* const, uintptr_t const);
static void map_page(Page_directory_table const* const, uint32_t const, uint32_t const, uintptr_t, uintptr_t);
static void map_page_area(Page_directory_table const* const, uint32_t const, uint32_t const, uintptr_t const, uintptr_t const, uintptr_t const, uintptr_t const);
static void map_page_same_area(Page_directory_table const* const, uint32_t const, uint32_t const, uintptr_t const, uintptr_t const);
static void unmap_page(Page_directory_table const* const, uintptr_t);
static void unmap_page_area(Page_directory_table const* const, uintptr_t const, uintptr_t const);
static List_node* list_get_new_page_node(void);
static void remove_page_list_node(List_node*);



void init_paging(Paging_data const * const pd) {
    kernel_pdt = pd->pdt;
    p_man = pd->p_man;
    p_info = pd->p_info;
    p_list_nodes = pd->p_list_nodes;
    used_list_node = pd->used_p_info;

    /* calculate and set page table addr to page directory entry */
    uintptr_t pt_base_addr = (vir_to_phys_addr((uintptr_t)kernel_pdt + (sizeof(Page_directory_entry) * (PAGE_DIRECTORY_ENTRY_NUM)))) >> PDE_PT_ADDR_SHIFT_NUM;
    for (size_t i = 0; i < PAGE_DIRECTORY_ENTRY_NUM; i++) {
        kernel_pdt[i].page_table_addr = (pt_base_addr + i) & 0xFFFFF;
    }

    /* Set kernel area paging and video area paging. */
    map_page_area(&kernel_pdt, PDE_FLAGS_KERNEL, PTE_FLAGS_KERNEL, get_kernel_vir_start_addr(), get_kernel_vir_end_addr(), get_kernel_phys_start_addr(), get_kernel_phys_end_addr());
    map_page_same_area(&kernel_pdt, PDE_FLAGS_KERNEL, PTE_FLAGS_KERNEL, 0xfd000000, 0xfd000000 + (600 * 800 * 4));

    /* Switch paging directory table. */
    set_cpu_pdt((uintptr_t)kernel_pdt);
    turn_off_4MB_paging();

    /* Inirialize page manager. */
    for (size_t i = 0; i < PAGE_INFO_NODE_NUM; ++i) {
        p_list_nodes[i].data = &p_info[i];
    }

    /* Set user space. */
    list_init(&p_man->user_area_list, sizeof(Page_info), NULL);
    List_node* n = list_get_new_page_node();
    Page_info* pp = (Page_info*)n->data;
    pp->base_addr = 0x00000000;
    pp->size = (size_t)get_kernel_vir_start_addr();
    pp->state = PAGE_INFO_STATE_FREE;
    list_insert_node_last(&p_man->user_area_list, n);

    /* Set kernel space. */
    list_init(&p_man->kernel_area_list, sizeof(Page_info), NULL);
    n = list_get_new_page_node();
    pp = (Page_info*)n->data;
    pp->base_addr = get_kernel_vir_start_addr();
    pp->size = get_kernel_size();
    pp->state = PAGE_INFO_STATE_ALLOC;
    list_insert_node_last(&p_man->kernel_area_list, n);

    n = list_get_new_page_node();
    pp = (Page_info*)n->data;
    pp->base_addr = get_kernel_vir_start_addr() + get_kernel_size();
    pp->size = 0xfd000000 - pp->base_addr;
    pp->state = PAGE_INFO_STATE_FREE;
    list_insert_node_last(&p_man->kernel_area_list, n);

    n = list_get_new_page_node();
    pp = (Page_info*)n->data;
    pp->base_addr = 0xfd000000;
    pp->size = 0xFFFFFFFF - 0xfd000000;
    pp->state = PAGE_INFO_STATE_ALLOC;
    list_insert_node_last(&p_man->kernel_area_list, n);
}


static size_t for_each_in_vmalloc_size;
static bool for_each_in_vmalloc(void* d) {
    Page_info const* const p = (Page_info*)d;
    return (p->state == PAGE_INFO_STATE_FREE && for_each_in_vmalloc_size <= p->size) ? true : false;
}


void* vmalloc(size_t size) {
    size = round_page_size(size);

    void* palloced = pmalloc(size);
    if (palloced == NULL) {
        return NULL;
    }

    /* search enough size node in kernel area. */
    for_each_in_vmalloc_size = size;
    List_node* n = list_for_each(&p_man->kernel_area_list, for_each_in_vmalloc, false);
    if (n == NULL) {
        pfree(palloced);
        return NULL;
    }

    /* get new node */
    List_node* new = list_get_new_page_node();
    if (new == NULL) {
        pfree(palloced);
        return NULL;
    }
    /* allocate new area */
    Page_info* pi = (Page_info*)n->data;
    Page_info* new_pi = (Page_info*)new->data;
    new_pi->base_addr = pi->base_addr + size;
    new_pi->size = pi->size - size;
    new_pi->state = PAGE_INFO_STATE_FREE;

    pi->size = size;
    pi->state = PAGE_INFO_STATE_ALLOC;

    list_insert_node_next(&p_man->kernel_area_list, n, new);

    map_page_area(&kernel_pdt, PDE_FLAGS_KERNEL & ~PDE_FLAG_GLOBAL, PTE_FLAGS_KERNEL & ~PTE_FLAG_GLOBAL, pi->base_addr, pi->base_addr + pi->size, (uintptr_t)palloced, (uintptr_t)palloced + size);

    return (void*)pi->base_addr;
}


static size_t for_each_in_vfree_base_addr;
static bool for_each_in_vfree(void* d) {
    Page_info const* const p = (Page_info*)d;
    return (p->state == PAGE_INFO_STATE_ALLOC && p->base_addr == for_each_in_vfree_base_addr) ? true : false;
}


void vfree(void* addr) {
    if (addr == NULL) {
        return;
    }

    for_each_in_vfree_base_addr = (uintptr_t)addr;
    List_node* n = list_for_each(&p_man->kernel_area_list, for_each_in_vfree, false);
    if (n == NULL) {
        return;
    }
    Page_info* n_pi = (Page_info*)n->data;

    /* fix page directory table infomation. */
    unmap_page_area(&kernel_pdt, n_pi->base_addr, n_pi->base_addr + n_pi->size);

    List_node* const prev_node = n->prev;
    List_node* const next_node = n->next;
    Page_info* const prev_pi = (Page_info*)prev_node->data;
    Page_info* const next_pi = (Page_info*)next_node->data;

    if (prev_pi->state == PAGE_INFO_STATE_FREE && prev_pi->state == PAGE_INFO_STATE_FREE) {
        /* merge next and previous. */
        prev_pi->size += (n_pi->size + next_pi->size);
        remove_page_list_node(n);
        remove_page_list_node(next_node);
    } else if (prev_pi->state == PAGE_INFO_STATE_FREE) {
        /* merge previous. */
        prev_pi->size += n_pi->size;
        remove_page_list_node(n);
    } else if (next_pi->state == PAGE_INFO_STATE_FREE) {
        /* merge next. */
        n_pi->size += next_pi->size;
        n_pi->state = PAGE_INFO_STATE_FREE;
        remove_page_list_node(next_node);
    } else {
        n_pi->state = PAGE_INFO_STATE_FREE;
    }
}


static List_node* list_get_new_page_node(void) {
    static size_t cnt = 0;         /* counter for nextfix */
    size_t const stored_cnt = cnt; /* It is used for detecting already checking all node. */

    do {
        cnt = MOD_MAX_PAGE_INFO_NODE_NUM(cnt + 1);
        if (stored_cnt == cnt) {
            /* all node is already used :( */
            return NULL;
        }
    } while (used_list_node[cnt] == true);

    used_list_node[cnt] = true;

    return &p_list_nodes[cnt];
}


static void remove_page_list_node(List_node* target) {
    list_remove_node(&p_man->kernel_area_list, target);

    /* instead of free(). */
    used_list_node[((uintptr_t)target - (uintptr_t)p_list_nodes) / sizeof(List_node)] = false;
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


static inline void unmap_page(Page_directory_table const* const pdt, uintptr_t vaddr) {
    Page_table_entry* const pte = get_pte(get_pt(get_pde(pdt, vaddr)), vaddr);
    pte->bit_expr &= PTE_FLAGS_AREA_MASK;

    flush_tlb(vaddr);
}


static inline void unmap_page_area(Page_directory_table const* const pdt, uintptr_t const begin_vaddr, uintptr_t const end_vaddr) {
    for (uintptr_t vaddr = begin_vaddr; vaddr < end_vaddr; vaddr += PAGE_SIZE) {
        unmap_page(pdt, vaddr);
    }
}


/*
 * static inline uintptr_t get_vaddr_from_pde_index(size_t const idx) {
 *     return idx <<  PDE_IDX_SHIFT_NUM;
 * }
 *
 *
 * static inline uintptr_t get_vaddr_from_pte_index(size_t const idx) {
 *     return idx <<  PTE_IDX_SHIFT_NUM;
 * }
 */



#include <stdio.h>
static bool p(void* d) {
    Page_info* p = (Page_info*)d;
    size_t mb = p->size / (1024 * 1024);
    printf("Base:%zx, Size:%zu%s, State:%s\n", p->base_addr, (mb == 0 ? p->size / 1024 : mb), (mb == 0 ? "KB" : "MB"), (p->state == PAGE_INFO_STATE_FREE ? "Free" : "Alloc"));
    return false;
}


void print_vmem(void) {
    puts("\n");
    list_for_each(&p_man->user_area_list, p, false);
    list_for_each(&p_man->kernel_area_list, p, false);

    puts("\n");
    size_t const size = 1024 * 1024;
    char* str = vmalloc(sizeof(char) * size);
    if (str == NULL) {
        puts("vmalloc is failed");
    } else {
        printf("addr: 0x%zx\n", (uintptr_t)str);
        /* 0xc0626000 */
        for (size_t i = 0; i < size; i++) {
            str[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % 26];
        }
    }

    vfree(str);

    /* This code must invokes Pagefault. */
    /* *str = '0'; */
}
