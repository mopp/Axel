/**
 * @file paging.c
 * @brief paging implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-05-20
 */

#include <asm_functions.h>
#include <assert.h>
#include <buddy.h>
#include <kernel.h>
#include <macros.h>
#include <memory.h>
#include <paging.h>
#include <tlsf.h>
#include <utils.h>
#include <proc.h>


/**
 * @brief Initialize paging.
 *          This is called in physical memory initialization.
 */
void init_paging(void) {
    Page_directory_table kernel_pdt = axel_s.kernel_pdt;

    memset(kernel_pdt, 0, ALL_PAGE_STRUCT_SIZE);

    /* Calculate and set page table addr to page directory entry */
    /* XXX: alignment sensitivity */
    uintptr_t pt_base_addr = (vir_to_phys_addr((uintptr_t)kernel_pdt + (sizeof(Page_directory_entry) * PAGE_DIRECTORY_ENTRY_NUM))) >> PDE_PT_ADDR_SHIFT_NUM;
    for (size_t i = 0; i < PAGE_DIRECTORY_ENTRY_NUM; i++) {
        kernel_pdt[i].page_table_addr = (pt_base_addr + i) & 0xFFFFF;
    }

    /* Set kernel area paging and video area paging. */
    map_page_area(kernel_pdt, PDE_FLAGS_KERNEL, PTE_FLAGS_KERNEL, get_kernel_vir_start_addr(), get_kernel_vir_end_addr(), get_kernel_phys_start_addr(), get_kernel_phys_end_addr());

    /* Switch paging directory table. */
    turn_off_pge();
    set_cpu_pdt(vir_to_phys_addr((uintptr_t)kernel_pdt));
    turn_off_4MB_paging();
    turn_on_pge();

    tlsf_init(axel_s.tman);
}


static inline size_t get_pde_index(uintptr_t const addr) {
    return (addr >> PDE_IDX_SHIFT_NUM) & 0x03FF;
}


static inline size_t get_pte_index(uintptr_t const addr) {
    return (addr >> PTE_IDX_SHIFT_NUM) & 0x03FF;
}


inline Page_directory_entry* get_pde(Page_directory_table const pdt, uintptr_t const vaddr) {
    return pdt + get_pde_index(vaddr);
}


inline Page_table get_pt(Page_directory_entry const* const pde) {
    return (Page_table)(phys_to_vir_addr((uintptr_t)(pde->page_table_addr << PDE_PT_ADDR_SHIFT_NUM)));
}


inline Page_table_entry* get_pte(Page_table const pt, uintptr_t const vaddr) {
    return pt + get_pte_index(vaddr);
}


static inline Page_table_entry* set_frame_addr(Page_table_entry* const pte, uintptr_t const paddr) {
    pte->frame_addr = (paddr >> PTE_FRAME_ADDR_SHIFT_NUM) & 0xFFFFF;
    return pte;
}


/**
 * @brief Mapping page at virtual addr to physical addr.
 *          And overwrite pde and pte flags.
 * @param pdt       Page_directory_table
 * @param pde_flags flags for Page_directory_entry.
 * @param pte_flags flags for Page_table_entry.
 * @param vaddr     virtual address.
 * @param paddr     physical address.
 */
inline void map_page(Page_directory_table pdt, uint32_t const pde_flags, uint32_t const pte_flags, uintptr_t vaddr, uintptr_t paddr) {
    Page_directory_entry* const pde = get_pde(pdt, vaddr);

    assert(pde->present_flag != 0);

    pde->bit_expr = (PDE_FLAGS_AREA_MASK & pde->bit_expr) | pde_flags;

    Page_table_entry* const pte = get_pte(get_pt(pde), vaddr);
    pte->bit_expr = (PTE_FLAGS_AREA_MASK & pte->bit_expr) | pte_flags;

    set_frame_addr(pte, paddr);
}


/**
 * @brief Mapping page area from begin_vaddr ~ end_vaddr to begin_paddr ~ end_paddr.
 *          And overwrite pde and pte flags.
 *          And size must be multiples of PAGE_SIZE.
 * @param pdt       Page_directory_table
 * @param pde_flags     Flags for Page_directory_entry.
 * @param pte_flags     Flags for Page_table_entry.
 * @param begin_vaddr   Begin virtual address of mapping area.
 * @param end_vaddr     End virtual address of mapping area.
 * @param begin_paddr   Begin physical address of mapping area.
 * @param end_paddr     End physical address of mapping area.
 */
inline void map_page_area(Page_directory_table pdt, uint32_t const pde_flags, uint32_t const pte_flags, uintptr_t const begin_vaddr, uintptr_t const end_vaddr, uintptr_t const begin_paddr, uintptr_t const end_paddr) {
    /* Generally speaking, PAGE_SIZE equals FRAME_SIZE. */
    /* size_t cnt = 0; */
    /* Page_table pt; */
    for (uintptr_t vaddr = begin_vaddr, paddr = begin_paddr; (vaddr < end_vaddr) && (paddr < end_paddr); vaddr += PAGE_SIZE, paddr += FRAME_SIZE) {
        map_page(pdt, pde_flags, pte_flags, vaddr, paddr);
        /* FIXME: vaddr_beginによる */
        // if (cnt++ == 0) {
        //     Page_directory_entry* pde = get_pde(pdt, vaddr);
        //     if (pde->present_flag == 0 && pdt != kernel_pdt) {
        //         /* Allocate page for user pdt */
        //         Page_table_entry* pt = vmalloc(sizeof(Page_table_entry) * PAGE_TABLE_ENTRY_NUM);
        //         pde->page_table_addr = ((vir_to_phys_addr((uintptr_t)pt) & 0xFFFFF000) >> PDE_PT_ADDR_SHIFT_NUM);
        //     }
        //     pde->bit_expr = (PDE_FLAGS_AREA_MASK & pde->bit_expr) | pde_flags;

        //     pt = get_pt(pde);
        // }

        // if (cnt == 9) {
        //     cnt = 0;
        // }

        // Page_table_entry* const pte = get_pte(pt, vaddr);
        // pte->bit_expr = (PTE_FLAGS_AREA_MASK & pte->bit_expr) | pte_flags;

        // set_frame_addr(pte, paddr);
    }
}


/**
 * @brief Mapping page area same physical address to virtual address.
 *          And overwrite pde and pte flags.
 *          And size must be multiples of PAGE_SIZE.
 * @param pdt       Page_directory_table
 * @param pde_flags     Flags for Page_directory_entry.
 * @param pte_flags     Flags for Page_table_entry.
 * @param begin_addr    Begin address of mapping area.
 * @param end_addr      End address of mapping area.
 */
inline void map_page_same_area(Page_directory_table pdt,  uint32_t const pde_flags, uint32_t const pte_flags, uintptr_t const begin_addr, uintptr_t const end_addr) {
    map_page_area(pdt, pde_flags, pte_flags, begin_addr, end_addr, begin_addr, end_addr);
}


inline void unmap_page(Page_directory_table pdt, uintptr_t vaddr) {
    if (KERNEL_VIRTUAL_BASE_ADDR <= vaddr) {
        /* Kernel area unmapping. */
        Page_directory_entry* const pde = get_pde(axel_s.kernel_pdt, vaddr);
        if (pde->present_flag == 0) {
            return;
        }

        Page_table_entry* const pte = get_pte(get_pt(pde), vaddr);
        if (pte->present_flag == 0) {
            return;
        }

        pte->bit_expr &= PTE_FLAGS_AREA_MASK;
        flush_tlb(vaddr);
    } else {
        /* User area unmapping. */
        /* TODO */
    }
}


inline void unmap_page_area(Page_directory_table pdt, uintptr_t const begin_vaddr, uintptr_t const end_vaddr) {
    for (uintptr_t vaddr = begin_vaddr; vaddr < end_vaddr; vaddr += PAGE_SIZE) {
        unmap_page(pdt, vaddr);
    }
}


Page_directory_table init_user_pdt(Page_directory_table pdt) {
    /* Copy kernel area. */
    memset(pdt, 0, sizeof(Page_directory_entry) * PAGE_DIRECTORY_ENTRY_NUM);
    size_t s = get_pde_index(get_kernel_vir_start_addr());
    /* size_t e = get_pde_index(get_kernel_vir_end_addr()); */
    size_t e = get_pde_index(0xffffffff);
    do {
        pdt[s].bit_expr = axel_s.kernel_pdt[s].bit_expr;
    } while (++s <= e);

    return pdt;
}


Page_directory_table get_kernel_pdt(void) {
    return axel_s.kernel_pdt;
}


inline bool is_kernel_pdt(Page_directory_table const pdt) {
    return (axel_s.kernel_pdt == pdt) ? true : false;
}


/* synchronize user and kernel pdt */
Axel_state_code synchronize_pdt(uintptr_t vaddr) {
    Process* p = get_current_pdt_process();
    if (p->pdt == NULL) {
        return AXEL_PAGE_SYNC_ERROR;
    }
    Page_directory_table user_pdt = p->pdt;
    Page_directory_entry* u_pde   = get_pde(user_pdt, vaddr);
    Page_directory_entry* k_pde   = get_pde(axel_s.kernel_pdt, vaddr);

    if (k_pde->present_flag == 0) {
        /*
         * Kernel space has not entry.
         * But, cause page fault in user pdt.
         * This is strange.
         */
        return AXEL_PAGE_SYNC_ERROR;
    }

    if (u_pde->present_flag == 0) {
        *u_pde = *k_pde;
    }

    // Page_table_entry* u_pte = get_pte(get_pt(u_pde), vaddr);
    // Page_table_entry* k_pte = get_pte(get_pt(k_pde), vaddr);
    // DIRECTLY_WRITE_STOP(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, k_pte);
    // if (k_pte->present_flag == 0) {
    //     /* This case is same things above */
    //     return AXEL_PAGE_SYNC_ERROR;
    // }

    // *u_pte = *k_pte;

    return AXEL_SUCCESS;
}


/*
 * Return head address of free virtual memorys
 * It has frame_nr * FRAME_SIZE size.
 */
static inline uintptr_t get_free_vmems(size_t frame_nr) {
    size_t pde_idx = get_pde_index(get_kernel_vir_end_addr());
    size_t pde_idx_limit = get_pde_index(0xf0000000);

    uintptr_t begin_vaddr = 0;
    size_t nr = 0;
    bool f = true;
    while (pde_idx < pde_idx_limit && f == true) {
        Page_table pt = get_pt(axel_s.kernel_pdt + pde_idx);
        for (size_t i = 0; i < PAGE_TABLE_ENTRY_NUM; i++) {
            if (pt[i].present_flag == 1) {
                begin_vaddr = 0;
                continue;
            }

            if (begin_vaddr == 0) {
                begin_vaddr = (pde_idx << PDE_IDX_SHIFT_NUM) | (i << PTE_IDX_SHIFT_NUM);
            }

            ++nr;

            if (frame_nr <= nr) {
                f = false;
                break;
            }
        }

        pde_idx++;
    }

    return begin_vaddr;
}


void* vcmalloc(Page* p, size_t size) {
    memset(p, 0, sizeof(Page));
    elist_init(&p->mapped_frames);

    /* allocate physical memory from BuddySystem. */
    uint8_t order = size_to_order(size);
    Frame* f = buddy_alloc_frames(axel_s.bman, order);
    if (f == NULL) {
        return NULL;
    }
    elist_insert_next(&p->mapped_frames, &f->list);
    size_t alloc_nr = p->frame_nr = (1 << f->order);

    /* allocate virtual memory from kernel page directory. */
    size = (FRAME_SIZE * alloc_nr);
    uintptr_t bvaddr = get_free_vmems(alloc_nr);
    if (bvaddr == 0) {
        buddy_free_frames(axel_s.bman, f);
        p->frame_nr = 0;
        return NULL;
    }
    p->addr          = bvaddr;
    uintptr_t evaddr = bvaddr + size;
    uintptr_t bpaddr = get_frame_addr(axel_s.bman, f);
    uintptr_t epaddr = bpaddr + size;
    map_page_area(axel_s.kernel_pdt, PDE_FLAGS_KERNEL_DYNAMIC, PTE_FLAGS_KERNEL_DYNAMIC, bvaddr, evaddr, bpaddr, epaddr);

    for (Frame* i = f; i < &f[alloc_nr]; i++, bvaddr += PAGE_SIZE) {
        i->mapped_vaddr = bvaddr;
    }

    return (void*)p->addr;
}


static inline void free_buddy_frames(Elist* head) {
    elist_foreach(i, head, Frame, list) {
        Frame* p = elist_derive(Frame, list, i->list.prev);
        elist_remove(&i->list);
        buddy_free_frames(axel_s.bman, i);
        i = p;
    }
}


void* vmalloc(Page* p, size_t size) {
    memset(p, 0, sizeof(Page));
    Elist* head = &p->mapped_frames;
    elist_init(head);

    uint8_t order   = size_to_order(size);
    size_t req_nr   = PO2(order);
    size_t alloc_nr = 0;

    do {
        Frame* f = buddy_alloc_frames(axel_s.bman, order);
        if (f != NULL) {
            alloc_nr += (1 << order);
            elist_insert_next(head, &f->list);
        }
        order--;
    } while (alloc_nr < req_nr && order < BUDDY_SYSTEM_MAX_ORDER);

    uintptr_t bvaddr  = get_free_vmems(alloc_nr);

    if (alloc_nr < req_nr || bvaddr == 0) {
        free_buddy_frames(&p->mapped_frames);
        memset(p, 0, sizeof(Page));
        return NULL;
    }
    p->frame_nr = alloc_nr;
    p->addr     = bvaddr;

    elist_foreach(i, head, Frame, list) {
        uintptr_t f_nr   = (1 << i->order);
        uintptr_t s      = (FRAME_SIZE * f_nr);
        uintptr_t bpaddr = get_frame_addr(axel_s.bman, i);
        uintptr_t epaddr = bpaddr + s;
        uintptr_t evaddr = bvaddr + s;

        map_page_area(axel_s.kernel_pdt, PDE_FLAGS_KERNEL_DYNAMIC, PTE_FLAGS_KERNEL_DYNAMIC, bvaddr, evaddr, bpaddr, epaddr);

        for (Frame* j = i; j < &i[f_nr]; j++, bvaddr += PAGE_SIZE) {
            j->mapped_vaddr = bvaddr;
        }
    }

    return (void*)p->addr;
}


void vfree(Page* p) {
    unmap_page_area(axel_s.kernel_pdt, p->addr, p->addr + (FRAME_SIZE * p->frame_nr));

    free_buddy_frames(&p->mapped_frames);
}


size_t size_to_frame_nr(size_t s) {
    /* FIXME: */
    return round_page_size(s) / FRAME_SIZE;
}


void* kmalloc(size_t s) {
    return tlsf_malloc(axel_s.tman, s);
}


void* kmalloc_zeroed(size_t s) {
    void* m = kmalloc(s);
    return (m == NULL) ? (NULL) : (memset(m, 0, s));
}


void kfree(void* p) {
    tlsf_free(axel_s.tman, p);
}


uintptr_t get_mapped_paddr(Page const * p) {
    return get_frame_addr(axel_s.bman, elist_derive(Frame, list, p->mapped_frames.next));
}


size_t get_page_size(Page const * const p ) {
    return p->frame_nr * FRAME_SIZE;
}


uintptr_t phys_to_vir_addr(uintptr_t addr) {
    return addr + KERNEL_VIRTUAL_BASE_ADDR;
}


uintptr_t vir_to_phys_addr(uintptr_t addr) {
    return addr - KERNEL_VIRTUAL_BASE_ADDR;
}


void set_phys_to_vir_addr(void* addr) {
    uintptr_t* p = (uintptr_t*)addr;
    *p = phys_to_vir_addr(*p);
}


size_t round_page_size(size_t size) {
    /*
     * Simple expression is below.
     * ((size + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE
     * In particular, PAGE_SIZE is power of 2.
     * We calculate same thing using below code.
     */
    return ((size + PAGE_SIZE - 1) & 0xFFFFFF000);
}
