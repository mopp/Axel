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
static uint16_t proc_id_cnt  = 0;

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
}


void switch_context(Interrupt_frame* current_iframe) {
    Process* current_p = processes[current_p_idx];
    Thread* current_t  = current_p->thread;
    Process* next_p    = processes[next_p_idx];
    Thread* next_t     = next_p->thread;
    current_t->iframe  = current_iframe;

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
        set_cpu_pdt(get_page_phys_addr(&next_p->pdt_page));
        pdt_process = next_p;
        axel_s.tss->esp0 = ECAST_UINT32(next_p->km_stack);
    }

    __asm__ volatile(
        "pushfl                             \n"   // Store eflags
        "movl  $next_turn, %[current_ip]    \n"   // Store ip
        "movl  %%esp,      %[current_sp]    \n"   // Store sp
        "movl  %[current_proc], %%ecx       \n"   // Set second argument
        "movl  %[next_proc],    %%edx       \n"   // Set first argument
        "movl  %[next_sp], %%esp            \n"   // Restore sp
        "pushl %[next_ip]                   \n"   // Restore ip (set return address)
        "jmp change_context                 \n"   // Change context

        ".globl next_turn                   \n"
        "next_turn:                         \n"

        "popfl                              \n"   // Restore eflags

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
        /*
         * We do not save page info.
         * Because, virtual address is repairable by frame.
         */
        Page pg;
        vcmalloc(&pg, (sizeof(Page_table_entry) * PAGE_TABLE_ENTRY_NUM));
        pde->page_table_addr = PAGE_TABLE_ADDR_MASK & (get_page_phys_addr(&pg) >> PDE_PT_ADDR_SHIFT_NUM);
    }

    Page_table_entry* const pte = get_pte(get_pt(pde), v_s);
    if (pte->present_flag == 1) {
        printf("overwrite area\n");
    }
    map_page_area(p->pdt, PDE_FLAGS_USER, PTE_FLAGS_USER, v_s, v_e, p_s, p_e);

    return AXEL_SUCCESS;
}


static inline Axel_state_code init_user_process(void) {
    Process* p        = kmalloc_zeroed(sizeof(Process));
    User_segments* us = &p->u_segs;
    elist_init(&p->used_pages);

    us->text.addr  = DEFAULT_TEXT_ADDR;
    us->stack.addr = DEFAULT_STACK_TOP_ADDR;
    p->pid         = proc_id_cnt++;
    p->km_stack    = (uintptr_t)(kmalloc_zeroed(KERNEL_MODE_STACK_SIZE)) + KERNEL_MODE_STACK_SIZE;
    p->thread      = kmalloc_zeroed(sizeof(Thread));
    p->thread->ip  = (uintptr_t)interrupt_return;
    p->thread->sp  = p->km_stack;

    /* alloc pdt */
    Page_directory_table pdt = vcmalloc(&p->pdt_page, sizeof(Page_directory_entry) * PAGE_DIRECTORY_ENTRY_NUM);
    p->pdt = init_user_pdt(pdt);

    /* setting user progam segments. */
    expand_segment(p, &us->text, DEFAULT_TEXT_SIZE);
    expand_segment(p, &us->stack, DEFAULT_STACK_SIZE);

    /* set pdt for setting init user space. */
    set_cpu_pdt(get_page_phys_addr(&p->pdt_page));

    p->thread->sp -= sizeof(Interrupt_frame);
    Interrupt_frame* intf = (Interrupt_frame*)p->thread->sp;
    p->thread->iframe = intf;
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
    /* static uint8_t inst[] = { 0xe8, 0x05, 0x00, 0x00, 0x00, 0xe9, 0xf6, 0xff, 0xff, 0xff, 0x35, 0xac, 0xac, 0x00, 0x00, 0xc3, }; */
    static uint8_t inst[] = {0xe8,  0x07 , 0x00 , 0x00 , 0x00 , 0xcd , 0x80 , 0xe9 , 0xf4 , 0xff , 0xff , 0xff , 0x35 , 0xac , 0xac , 0x00 , 0x00 , 0xc3};
    for (size_t i = 0; i < ARRAY_SIZE_OF(inst); i++) {
        *((uint8_t*)us->text.addr + i) = inst[i];
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
