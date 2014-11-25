/**
 * @file proc.c
 * @brief This is process function implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-07-14
 */

#include <proc.h>
#include <stdint.h>
#include <utils.h>
#include <macros.h>
#include <asm_functions.h>
#include <segment.h>
#include <kernel.h>
#include <interrupt.h>
#include <assert.h>
#include <fs.h>
#include <elf.h>


enum Process_constants {
    PROC_STATE_FREE,
    PROC_STATE_ALLOC,
    PROC_STATE_RUN,
    PROC_STATE_SLEEP,
    PROC_STATE_WAIT,
    PROC_STATE_ZOMBI,
    KERNEL_MODE_STACK_SIZE = FRAME_SIZE,
    MAX_PROC_NR = 255,
};


bool is_enable_process = false;
static Process* pdt_process;
static Process* run_proc;
static Process procs[MAX_PROC_NR];


Process* pdt_proc(void) {
    return pdt_process;
}

static Process* alloc_proc(void) {
    static uint16_t pid = 0;
    static size_t last = 0;
    size_t store = last;
    Process* p = NULL;

    do {
        if (procs[last].state == PROC_STATE_FREE) {
            p = &procs[last];
            memset(p, 0, sizeof(Process));
            p->state = PROC_STATE_ALLOC;
            p->pid   = pid++;
        }

        last++;
        if (MAX_PROC_NR == last) {
            last = 0;
        }

        if (store == last) {
            /* Executable process is not found. */
            return NULL;
        }
    } while(p == NULL);

    return p;
}


static inline void free_proc(Process* p) {
    if (p == NULL) {
        return;
    }

    p->state = PROC_STATE_FREE;
}


static Process* next_proc(void) {
    static size_t last = 0;
    size_t store = last;
    while (1) {
        last++;
        if (MAX_PROC_NR == last) {
            last = 0;
        }
        if (store == last) {
            /* Executable process is not found. */
            return NULL;
        }

        if (procs[last].state == PROC_STATE_RUN) {
            return &procs[last];
        }
    }
}


Process* running_proc(void) {
    return run_proc;
}


/**
 * @brief This function is "jumpled"(NOT called) by switch_context.
 *          And Stack(esp) is already chenged next stack.
 * @param current pointer to current process.
 * @param next pointer to next process.
 */
void __fastcall change_context(Process* current, Process* next) {
    run_proc= next;
}


void switch_context(Interrupt_frame* current_iframe) {
    Process* current_p = running_proc();
    Process* next_p    = next_proc();
    if (current_p == next_p) {
        return;
    }

    Thread* current_t  = &current_p->thread;
    Thread* next_t     = &next_p->thread;
    current_t->iframe  = current_iframe;

    if (next_p->pdt != NULL) {
        /*
         * Next Memory space is user space.
         * So, change pdt.
         */
        set_cpu_pdt(get_page_phys_addr(&next_p->pdt_page));
        pdt_process = next_p;
        axel_s.tss->esp0 = ECAST_UINT32(next_p->km_stack);
    }

    __asm__ volatile(
            "pushfl                             \n"  // Store eflags
            "movl  $next_turn, %[current_ip]    \n"  // Store ip
            "movl  %%esp,      %[current_sp]    \n"  // Store sp
            "movl  %[current_proc], %%ecx       \n"  // Set second argument
            "movl  %[next_proc],    %%edx       \n"  // Set first argument
            "movl  %[next_sp], %%esp            \n"  // Restore sp
            "pushl %[next_ip]                   \n"  // Restore ip (set return address)
            "jmp change_context                 \n"  // Change context

            ".globl next_turn                   \n"
            "next_turn:                         \n"

            "popfl                              \n"  // Restore eflags

            :   [current_ip] "=m"(current_t->ip),
                [current_sp] "=m"(current_t->sp)
            :   [next_ip] "r"(next_t->ip),
                [next_sp] "r"(next_t->sp),
                [current_proc] "m"(current_p),
                [next_proc] "m"(next_p)
            :   "memory", "%ecx", "%edx"
            );
}


/* FIXME: */
static inline Axel_state_code expand_segment(Process* p, Segment* s, size_t size) {
    Frame* f = buddy_alloc_frames(axel_s.bman, size_to_order(size));
    if (f == NULL) {
        return AXEL_FRAME_ALLOC_ERROR;
    }

    size = PO2(f->order) * FRAME_SIZE;
    uintptr_t p_s = frame_to_phys_addr(axel_s.bman, f);
    uintptr_t p_e = p_s + size;
    uintptr_t v_s = s->addr + s->size;
    uintptr_t v_e = v_s + size;
    f->mapped_kvaddr = v_s;
    s->size += size;

    /* allocate page table */
    Page_directory_entry* const pde = get_pde(p->pdt, v_s);
    if (pde->present_flag == 0) {
        Page* pg = kmalloc(sizeof(Page));
        vcmalloc(pg, (sizeof(Page_table_entry) * PAGE_TABLE_ENTRY_NUM));
        pde->page_table_addr = PAGE_TABLE_ADDR_MASK & (get_page_phys_addr(pg) >> PDE_PT_ADDR_SHIFT_NUM);
        elist_insert_next(&p->used_pages, &pg->list);
    }

    Page_table_entry* const pte = get_pte(get_pt(pde), v_s);
    if (pte->present_flag == 1) {
        return AXEL_FAILED;
    }
    map_page_area(p->pdt, PDE_FLAGS_USER, PTE_FLAGS_USER, v_s, v_e, p_s, p_e);

    return AXEL_SUCCESS;
}


static inline Axel_state_code init_user_process(void) {
    Process* p = alloc_proc();
    User_segments* us = &p->u_segs;
    elist_init(&p->used_pages);

    us->text.addr  = DEFAULT_TEXT_ADDR;
    us->text.size  = 0;
    us->stack.addr = DEFAULT_STACK_TOP_ADDR;
    us->stack.size = 0;
    p->km_stack    = (uintptr_t)(kmalloc_zeroed(KERNEL_MODE_STACK_SIZE)) + KERNEL_MODE_STACK_SIZE;
    p->thread.ip   = (uintptr_t)interrupt_return;
    p->thread.sp   = p->km_stack;

    /* alloc pdt */
    Page_directory_table pdt = vcmalloc(&p->pdt_page, sizeof(Page_directory_entry) * PAGE_DIRECTORY_ENTRY_NUM);
    p->pdt = init_user_pdt(pdt);

    /* setting user progam segments. */
    expand_segment(p, &us->text, DEFAULT_TEXT_SIZE);
    expand_segment(p, &us->stack, DEFAULT_STACK_SIZE);

    p->thread.sp -= sizeof(Interrupt_frame);
    Interrupt_frame* intf = (Interrupt_frame*)p->thread.sp;
    p->thread.iframe = intf;
    memset(intf, 0, sizeof(Interrupt_frame));

    intf->ds       = USER_DATA_SEGMENT_SELECTOR;
    intf->es       = USER_DATA_SEGMENT_SELECTOR;
    intf->fs       = USER_DATA_SEGMENT_SELECTOR;
    intf->gs       = USER_DATA_SEGMENT_SELECTOR;
    intf->eip      = ECAST_UINT32(us->text.addr);
    intf->cs       = USER_CODE_SEGMENT_SELECTOR;
    intf->eflags   = 0x00000200;
    intf->prev_esp = ECAST_UINT32(us->stack.addr + DEFAULT_STACK_SIZE);
    intf->prev_ss  = USER_DATA_SEGMENT_SELECTOR;

    /* dirty initialize. */
    /* static uint8_t inst[] = { */
        /* 0xe8,0x14,0x00,0x00,0x00,0xb8,0x00,0x00,0x00,0x00,0x68,0x1f,0x10,0x00,0x00,0xcd,0x80,0x83,0xc4,0x04,0xe9,0xe7,0xff,0xff,0xff,0x35,0xac,0xac,0x00,0x00,0xc3,0x4d,0x4f,0x50,0x50,0x00 */
    /* }; */
    /* memcpy((void*)us->text.addr, inst, ARRAY_SIZE_OF(inst)); */

    /* Load program file. */
    File* f = resolve_path(axel_s.fs, "init");
    void* fbuf = kmalloc(f->size);
    if (f->belong_fs->access_file(FILE_READ, f, fbuf) != AXEL_SUCCESS ) {
        kfree(fbuf);
        return AXEL_FAILED;
    }

    /* set pdt for setting init user space. */
    set_cpu_pdt(get_page_phys_addr(&p->pdt_page));

    memcpy((void*)us->text.addr, fbuf, f->size);

    set_cpu_pdt(vir_to_phys_addr((uintptr_t)(get_kernel_pdt())));

    p->state = PROC_STATE_RUN;

    return AXEL_SUCCESS;
}


static inline void free_page(Elist* used_pages, uintptr_t addr) {
    Page* freep = NULL;
    elist_foreach(p, used_pages, Page, list) {
        if (p->addr == addr) {
            vfree(p);
            freep = p;
            break;
        }
    }

    if (freep != NULL) {
        elist_remove(&freep->list);
        kfree(freep);
    }
}


/* FIXME: over 4MB */
static inline void free_segment(Process* p, Segment* s) {
    size_t size    = s->size;
    uintptr_t addr = s->addr;
    uintptr_t lim  = addr + size;

    Page_directory_entry* const pde = get_pde(p->pdt, addr);
    if (pde->present_flag != 0) {
        for (uintptr_t a = addr; a < lim; a+= FRAME_SIZE) {
            Page_table_entry* const pte = get_pte(get_pt(pde), a);
            memset(pte, 0, sizeof(Page_table_entry));
            unmap_page(a);
        }
        free_page(&p->used_pages, addr);
        pde->page_table_addr = 0;
    }
}


Axel_state_code elf_callback(void* o, size_t n, void const* fbuf, Elf_phdr const* ph) {
    Process* p = (Process*)o;
    Segment* s = NULL;
    switch (n) {
        case 0:
            s = &p->u_segs.text;
            break;
        case 1:
            s = &p->u_segs.data;
            break;
        default:
            return AXEL_SUCCESS;
    }

    s->addr = ALIGN_DOWN(ph->vaddr, FRAME_SIZE);
    s->size = 0;
    size_t size = ALIGN_UP(ph->memsz + (ph->vaddr - s->addr), FRAME_SIZE);
    expand_segment(p, s, size);

    set_cpu_pdt(get_page_phys_addr(&p->pdt_page));
    /* Copy segment into memory. */
    void* segbuf = (void*)((uintptr_t)fbuf + ph->offset);
    memcpy((void*)s->addr, segbuf, s->size);

    return AXEL_SUCCESS;
}


Axel_state_code elf_callback_error(void* p, size_t n, void const* fbuf, Elf_phdr const* ph) {
    /* TODO: */
    return AXEL_SUCCESS;
}


Axel_state_code execve(char const *path, char const * const *argv, char const * const *envp) {
    if (path == NULL) {
        return AXEL_FAILED;
    }

    /* Process* p = running_proc(); */
    Process* p = &procs[1];

    /* Load program file. */
    File* f = resolve_path(axel_s.fs, path);
    void* fbuf = kmalloc(f->size);
    if (f->belong_fs->access_file(FILE_READ, f, fbuf) != AXEL_SUCCESS ) {
        kfree(fbuf);
        return AXEL_FAILED;
    }

    io_cli();

    /* Free current program on memory. */
    User_segments* segs = &p->u_segs;
    if (segs->text.addr != 0) {
        free_segment(p, &segs->text);
    }
    if (segs->data.addr != 0) {
        free_segment(p, &segs->data);
    }

    /* Load program into memory. */
    Axel_state_code r = elf_load_program(fbuf, p, elf_callback, elf_callback_error);
    if (r != AXEL_SUCCESS) {
        return AXEL_FAILED;
    }

    /* Init stack pointer. */
    p->thread.iframe->prev_esp = p->u_segs.stack.addr + p->u_segs.stack.size;
    p->thread.iframe->eip = elf_get_entry_addr(fbuf);

    io_sti();

    return AXEL_SUCCESS;
}


Axel_state_code init_process(void) {
    /* Current context is dealed with process 0. */
    run_proc = alloc_proc();
    run_proc->state = PROC_STATE_RUN;

    init_user_process();

    is_enable_process = true;

    return AXEL_SUCCESS;
}
