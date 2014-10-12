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




static Process pa, pb, pk;
static Process** processes;
static Process* init_user_p;
static uint8_t process_num   = 2;
static uint8_t current_p_idx = 0;
static uint8_t next_p_idx    = 1;

bool is_enable_process = false;


static Process* running_process;
Process* get_current_process(void) {
    return running_process;
}


static Process* pdt_process;
Process* get_current_pdt_process(void) {
    return pdt_process;
}


/**
 * @brief This function is "jumpled"(NOT called) by switch_context.
 *          And Stack(esp) is already chenged next stack.
 * @param current pointer to current process.
 * @param next pointer to next process.
 */
void __fastcall change_context(Process* current, Process* next) {
    io_sti();
}


void switch_context(Interrupt_frame* current_iframe) {
    io_cli();
    Process* current_p = processes[current_p_idx];
    Thread* current_t  = current_p->thread;
    Process* next_p    = processes[next_p_idx];
    Thread* next_t     = next_p->thread;
    current_t->iframe = current_iframe;

    current_p_idx++;
    if (process_num <= current_p_idx) {
        current_p_idx = 0;
    }

    next_p_idx++;
    if (process_num <= next_p_idx) {
        next_p_idx = 0;
    }

    if (next_p->pdt != NULL) {
        /*
         * Next Memory space is user space.
         * So, change pdt.
         */
        io_cli();
        set_cpu_pdt(get_mapped_paddr(&next_p->pdt_page));
        pdt_process = next_p;
        io_sti();
        axel_s.tss->esp0 = ECAST_UINT32(next_t->iframe->prev_esp);
    }

    __asm__ volatile(
        "pushl %%ebp                        \n\t"
        "pushfl                             \n\t"   // Store eflags
        "movl  $next_turn, %[current_ip]    \n\t"   // Store ip
        "movl  %%esp,      %[current_sp]    \n\t"   // Store sp
        "movl  %[current_proc], %%ecx       \n\t"   // Set second argument
        "movl  %[next_proc],    %%edx       \n\t"   // Set first argument
        "movl  %[next_sp], %%esp            \n\t"   // Restore sp
        "pushl %[next_ip]                   \n\t"   // Restore ip (set return address)
        "jmp change_context                 \n\t"   // Change context

        ".globl next_turn                   \n\t"
        "next_turn:                         \n\t"

        "popfl                              \n\t"   // Restore eflags
        "popl  %%ebp                        \n\t"

        :   [current_ip]  "=m" (current_t->ip),
            [current_sp]  "=m" (current_t->sp)
        :   [next_ip]      "r" (next_t->ip),
            [next_sp]      "r" (next_t->sp),
            [current_proc] "m" (current_p),
            [next_proc]    "m" (next_p)
        : "memory", "%ecx", "%edx"
    );
}


_Noreturn void task_a(void) {
    while (1) {
        *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 'A';
        /* puts("Task A\n"); */
    }
}


_Noreturn void task_b(void) {
    while (1) {
        *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 'B';
        /* puts("Task B\n"); */
    }
}


/* FIXME: */
static inline Axel_state_code expand_segment(Process* p, Segment* s, size_t size) {
    Frame* f = buddy_alloc_frames(axel_s.bman, size_to_order(size));
    if (f == NULL) {
        return AXEL_FRAME_ALLOC_ERROR;
    }
    elist_insert_next(&s->mapped_frames, &f->list);

    size = PO2(f->order) * FRAME_SIZE;
    uintptr_t p_s = get_frame_addr(axel_s.bman, f);
    uintptr_t p_e = p_s + size;
    uintptr_t v_s = s->addr + s->size;
    uintptr_t v_e = v_s + size;
    f->mapped_vaddr = v_s;
    s->size += size;

    /* allocate page table */
    Page_directory_entry* pde = get_pde(p->pdt, v_s);
    if (pde->present_flag == 0) {
        Page* pg = kmalloc_zeroed(sizeof(Page));
        vcmalloc(pg, (sizeof(Page_table_entry) * PAGE_TABLE_ENTRY_NUM));
        pde->page_table_addr = ((pg->addr & 0xfffff) >> PDE_PT_ADDR_SHIFT_NUM);
        elist_insert_next(&p->used_pages, &pg->list);
    }

    map_page_area(p->pdt, PDE_FLAGS_USER, PTE_FLAGS_USER, v_s, v_e, p_s, p_e);

    return AXEL_SUCCESS;
}


Axel_state_code init_user_process(void) {
    Process* p        = kmalloc_zeroed(sizeof(Process));
    User_segments* ps = kmalloc_zeroed(sizeof(User_segments));

    elist_init(&p->used_pages);
    elist_init(&ps->text.mapped_frames);
    elist_init(&ps->data.mapped_frames);
    elist_init(&ps->stack.mapped_frames);

    ps->text.addr     = DEFAULT_TEXT_ADDR;
    ps->stack.addr    = DEFAULT_STACK_TOP_ADDR;
    p->segments       = ps;
    p->pid            = 0;
    p->thread         = kmalloc_zeroed(sizeof(Thread));
    p->thread->ip     = (uintptr_t)interrupt_return;

    /* alloc pdt */
    Page_directory_table pdt = vcmalloc(&p->pdt_page, sizeof(Page_directory_entry) * PAGE_DIRECTORY_ENTRY_NUM);
    p->pdt = init_user_pdt(pdt);

    expand_segment(p, &p->segments->text, DEFAULT_TEXT_SIZE); /* Text segment */
    expand_segment(p, &p->segments->stack, DEFAULT_STACK_SIZE); /* Stack segment */
    p->thread->sp = p->segments->stack.addr + DEFAULT_STACK_SIZE;

    /* set pdt for setting init user space. */
    set_cpu_pdt(get_mapped_paddr(&p->pdt_page));

    p->thread->sp -= sizeof(Interrupt_frame);
    Interrupt_frame* intf = (Interrupt_frame*)p->thread->sp;
    memset(intf, 0, sizeof(Interrupt_frame));

    intf->ds          = USER_DATA_SEGMENT_SELECTOR;
    intf->es          = USER_DATA_SEGMENT_SELECTOR;
    intf->fs          = USER_DATA_SEGMENT_SELECTOR;
    intf->gs          = USER_DATA_SEGMENT_SELECTOR;
    intf->eip         = ECAST_UINT32(ps->text.addr);
    intf->cs          = USER_CODE_SEGMENT_SELECTOR;
    intf->eflags      = 0x00000200;
    intf->prev_esp    = ECAST_UINT32(p->segments->stack.addr + DEFAULT_STACK_SIZE);
    intf->prev_ss     = USER_DATA_SEGMENT_SELECTOR;
    p->thread->iframe = intf;

    /* dirty initialize. */
    static uint8_t inst[] = {0x35, 0xf5, 0xf5, 0x00, 0x00, 0xe9, 0xf6, 0xff, 0xff, 0xff};
    for (size_t i = 0; i < ARRAY_SIZE_OF(inst); i++) {
        *((uint8_t*)ps->text.addr + i) = inst[i];
    }

    set_cpu_pdt(vir_to_phys_addr((uintptr_t)(get_kernel_pdt())));

    init_user_p = p;

    return AXEL_SUCCESS;
}


Axel_state_code init_process(void) {
    pk.pid = 0;
    pk.pdt = NULL;
    pk.thread = kmalloc(sizeof(Thread));

    pa.pid = 1;
    pa.pdt = NULL;
    pa.thread = kmalloc(sizeof(Thread));
    void* p = kmalloc(0x1000);
    pa.thread->sp = (uintptr_t)p + 0x1000;
    pa.thread->ip = (uintptr_t)task_a;

    pb.pid = 2;
    pb.pdt = NULL;
    pb.thread = kmalloc(sizeof(Thread));
    p = kmalloc(0x1000);
    pb.thread->sp = (uintptr_t)p + 0x1000;
    pb.thread->ip = (uintptr_t)task_b;

    init_user_process();

    processes = kmalloc(sizeof(Process*) * process_num);
    processes[0] = &pk;
    processes[1] = init_user_p;

    is_enable_process = true;

    return AXEL_SUCCESS;
}
