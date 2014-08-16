/**
 * @file proc.c
 * @brief This is process function implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-07-14
 */

#include <proc.h>
#include <stdint.h>
#include <string.h>
#include <macros.h>
#include <paging.h>
#include <asm_functions.h>


struct task_state_segment {
    uint16_t back_link;
    uint16_t reserved0;
    uint32_t esp0; /* stack segment and stack pointer register value of privileged level 0 */
    uint16_t ss0, reserved1;
    uint32_t esp1; /* stack segment and stack pointer register value of privileged level 1 */
    uint16_t ss1, reserved2;
    uint32_t esp2; /* stack segment and stack pointer register value of privileged level 2 */
    uint16_t ss2, reserved3;
    uint32_t cr3;    /* page directly base register value. */
    uint32_t eip;    /* eip register value before task switching. */
    uint32_t eflags; /* eflags register value before task switching. */
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t es, reserved4;
    uint16_t cs, reserved5;
    uint16_t ss, reserved6;
    uint16_t ds, reserved7;
    uint16_t fs, reserved8;
    uint16_t gs, reserved9;
    uint16_t ldtr, reserved10;
    uint16_t debug_trap_flag : 1;
    uint16_t reserved11 : 15;
    uint16_t iomap_base_addr;
};
typedef struct task_state_segment Task_state_segment;
_Static_assert(sizeof(Task_state_segment) == 104, "Task_state_segment size is NOT 104 byte");


struct thread {
    uintptr_t sp; /* Stack pointer. */
    uintptr_t ip; /* Instruction pointer. */
};
typedef struct thread Thread;


struct program_sections {
    uintptr_t text_addr;
    uintptr_t data_addr;
    uintptr_t heap_addr;
};
typedef struct program_sections Program_sections;


struct process {
    uint8_t priority;
    uint8_t state;
    uint16_t pid;
    uint32_t cpu_time;
    Program_sections* p_sections;
    Thread* thread;
    Page_directory_table pdt; /* memory space. */
};
typedef struct process Process;



static Process pa, pb, pk, pu;
static Process** processes;
static uint8_t process_num = 3;
static uint8_t current_p_idx = 0;
static uint8_t next_p_idx = 1;


/* TODO: */
Process* get_current_process(void) {
    return NULL;
}


void dummy(void) {
    return;
}


/* TODO: chenge page global directly */
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
        "pushfl                             \n\t"   // store eflags
        "pushl %%ebp                        \n\t"
        "pushl %%eax                        \n\t"
        "pushl %%ecx                        \n\t"
        "pushl %%edx                        \n\t"
        "pushl %%ebx                        \n\t"

        "movl  $next_turn, %[current_ip]    \n\t"   // store ip
        "movl  %%esp, %[current_sp]         \n\t"   // store sp
        "movl  %[next_sp], %%esp            \n\t"   // restore sp
        "pushl %[next_ip]                   \n\t"   // restore ip
        "jmp   dummy                        \n\t"   // change context

        "next_turn:                         \n\t"

        "popl %%ebx                         \n\t"
        "popl %%edx                         \n\t"
        "popl %%ecx                         \n\t"
        "popl %%eax                         \n\t"
        "popl %%ebp                         \n\t"
        "popfl                              \n\t"   // restore eflags

        :   [current_ip] "=m" (current_t->ip),
            [current_sp] "=m" (current_t->sp)
        :   [next_ip] "m" (next_t->ip),
            [next_sp] "m" (next_t->sp)
        : "memory"
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


Axel_state_code init_process(void) {
    pk.pid = 0;
    pk.thread = vmalloc(sizeof(Thread));

    pa.pid = 1;
    pa.thread = vmalloc(sizeof(Thread));
    pa.thread->sp = vmalloc_addr(0x1000);
    pa.thread->ip = (uintptr_t) task_a;

    pb.pid = 2;
    pb.thread = vmalloc(sizeof(Thread));
    pb.thread->sp = vmalloc_addr(0x1000);
    pb.thread->ip = (uintptr_t) task_b;

    // pu.context = vmalloc(sizeof(Context));
    // pu.context->ip = (uint32_t)(uintptr_t) task_user;
    // pu.context->sp = (uint32_t)(uintptr_t) pb.stack;
    // pu.context->pdt = make_user_pdt();
    // pu.pid = 2;
    /* pu.stack        = uvmalloc(0x1000, &pu.context->pdt); */
    /* 0:	f4                   	hlt     */
    /* 1:	eb fd                	jmp    0 <usr> */

    processes = vmalloc(sizeof(Process*) * process_num);
    processes[0] = &pk;
    processes[1] = &pa;
    processes[2] = &pb;
    /* processes[3] = &pu; */

    return AXEL_SUCCESS;
}
