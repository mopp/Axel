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
#include <paging.h>
#include <memory.h>
#include <asm_functions.h>
#include <segment.h>



struct thread {
    uintptr_t sp; /* Stack pointer. */
    uintptr_t ip; /* Instruction pointer. */
};
typedef struct thread Thread;


/* Each size is rounded by page size. */
struct program_segments {
    uintptr_t text_addr;
    uintptr_t data_addr;
    uintptr_t heap_addr;
    uintptr_t stack_addr;
    size_t text_size;
    size_t data_size;
    size_t heap_size;
    size_t stack_size;
};
typedef struct program_segments Program_segments;


enum process_constants {
    PSTATE_RUN,
    PSTATE_SLEEP,
    PSTATE_WAIT,
    PSTATE_ZOMBI,

    INIT_USER_STACK_SIZE = 1024,
};


struct process {
    uint8_t priority;
    uint8_t state;
    uint16_t pid;
    uint32_t cpu_time;
    Program_segments* segments;
    Thread* thread;
    Page_directory_table pdt; /* Own memory space. */
};
typedef struct process Process;



static Process pa, pb, pk, pu;
static Process** processes;
static uint8_t process_num   = 3;
static uint8_t current_p_idx = 0;
static uint8_t next_p_idx    = 1;

bool is_enable_process = false;


/* TODO: */
Process* get_current_process(void) {
    return NULL;
}


/**
 * @brief This function is "jumpled"(NOT called) by switch_context.
 *          And Stack(esp) is already chenged next stack.
 * @param current pointer to current process.
 * @param next pointer to next process.
 */
void __fastcall change_context(Process* current, Process* next) {
    DIRECTLY_WRITE(uintptr_t, KERNEL_VIRTUAL_BASE_ADDR, current->pid);

    if (next->pdt != NULL) {
        /*
         * Next Memory space is user space.
         * So, change pdt.
         */
        BOCHS_MAGIC_BREAK()
        set_cpu_pdt(next->pdt);

        /* dirty initialize. */
        static uint8_t inst[] = {0x35, 0xf5, 0xf5, 0x00, 0x00, 0xe9, 0xf6, 0xff, 0xff, 0xff};
        for (size_t i = 0; i < ARRAY_SIZE_OF(inst); i++) {
            *((uint8_t*)next->thread->ip + i) = inst[i];
        }
    }
}


void switch_context(void) {
    Process* current_p = processes[current_p_idx];
    Thread* current_t  = current_p->thread;
    Process* next_p    = processes[next_p_idx];
    Thread* next_t     = next_p->thread;

    current_p_idx++;
    if (process_num <= current_p_idx) {
        current_p_idx = 0;
    }

    next_p_idx++;
    if (process_num <= next_p_idx) {
        next_p_idx = 0;
    }

    __asm__ volatile(
        "pushl %%ebp                        \n\t"
        "pushfl                             \n\t"   // Store eflags

        "movl  $next_turn, %[current_ip]    \n\t"   // Store ip
        "movl  %%esp,      %[current_sp]    \n\t"   // Store sp
        "movl  %[next_sp], %%esp            \n\t"   // Restore sp

        "movl  %[current_proc], %%ecx       \n\t"   // Set second argument
        "movl  %[next_proc],    %%edx       \n\t"   // Set first argument
        "pushl %[next_ip]                   \n\t"   // Restore ip (set return address)
        "jmp   change_context               \n\t"   // Change context

        ".globl next_turn                   \n\t"
        "next_turn:                         \n\t"

        "popfl                              \n\t"   // Restore eflags
        "popl  %%ebp                        \n\t"

        :   [current_ip]  "=m" (current_t->ip),
            [current_sp]  "=m" (current_t->sp)
        :   [next_ip]      "m" (next_t->ip),
            [next_sp]      "m" (next_t->sp),
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


_Noreturn void task_user(void) {
    while (1) {
        /* *(uint32_t*)1 = 'U'; */
        /* puts("Task User\n"); */
    }
}


Process* make_user_process(void) {
    Process* p = vmalloc(sizeof(Process));
    Program_segments* ps = vmalloc(sizeof(Program_segments));

    p->cpu_time    = 0;
    p->segments    = ps;
    ps->text_addr  = 0x1000;
    ps->text_size  = 0x1000;
    ps->data_addr  = 0;
    ps->data_size  = 0;
    ps->heap_addr  = 0;
    ps->heap_size  = 0;
    ps->stack_addr = 0x2000;
    ps->stack_size = INIT_USER_STACK_SIZE;
    p->pdt         = make_user_pdt();
    p->pid         = 100;
    p->thread      = vmalloc(sizeof(Thread));
    p->thread->ip  = 0x1000; /* XXX: start virtual address 0x1000; */
    p->thread->sp  = ps->stack_addr + ps->stack_size;

    /* Init user space. */
    if (0 != ps->text_size) {
        /* Text segment */
        uintptr_t t   = vir_to_phys_addr(vmalloc_addr(ps->text_size));
        uintptr_t p_s = t;
        uintptr_t p_e = p_s + ps->text_size;
        uintptr_t v_s = ps->text_addr;
        uintptr_t v_e = v_s + ps->text_size;
        map_page_area(p->pdt, PDE_FLAGS_USER, PTE_FLAGS_USER, v_s, v_e, p_s, p_e);

        /* Stack segment */
        t   = vir_to_phys_addr(vmalloc_addr(ps->stack_size));
        p_s = t;
        p_e = p_s + ps->stack_size;
        v_s = ps->stack_addr;
        v_e = v_s + ps->stack_size;
        map_page_area(p->pdt, PDE_FLAGS_USER, PTE_FLAGS_USER, v_s, v_e, p_s, p_e);
    }

    return p;
}


Axel_state_code init_process(void) {
    is_enable_process = true;

    pk.pid = 0;
    pk.pdt = NULL;
    pk.thread = vmalloc(sizeof(Thread));

    pa.pid = 1;
    pa.pdt = NULL;
    pa.thread = vmalloc(sizeof(Thread));
    pa.thread->sp = vmalloc_addr(0x1000);
    pa.thread->ip = (uintptr_t) task_a;

    pb.pid = 2;
    pb.pdt = NULL;
    pb.thread = vmalloc(sizeof(Thread));
    pb.thread->sp = vmalloc_addr(0x1000);
    pb.thread->ip = (uintptr_t) task_b;

    pu = *(make_user_process());
    /* 0:	f4                   	hlt     */
    /* 1:	eb fd                	jmp    0 <usr> */

    processes = vmalloc(sizeof(Process*) * process_num);
    processes[0] = &pk;
    processes[1] = &pa;
    processes[2] = &pb;
    /* processes[3] = &pu; */

    return AXEL_SUCCESS;
}
