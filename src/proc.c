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
typedef struct task_state_segment Context;
_Static_assert(sizeof(Task_state_segment) == 104, "Task_state_segment size is NOT 104 byte");

struct segment_selector {
    uint16_t request_privilege_level : 2;
    uint16_t table_indicator : 1;
    uint16_t index : 13;
};
typedef struct segment_selector Segment_selector;
_Static_assert(sizeof(Segment_selector) == 2, "Segment_selector size is NOT 16bit");

struct process {
    uint8_t priority;
    uint16_t pid;
    uint32_t live_time;
    Context* context;
    char const * name;
    void* stack;
};
typedef struct process Process;



static Process pa, pb, pk;
/* static Process* current; */
static char const* name_a = "Process A";
static char const* name_b = "Process B";
static char const* name_k = "Process Kernel";
static Process** processes;
static uint8_t current_p_idx = 0;
static uint8_t next_p_idx = 1;


#define OFFSETOF(type, field)    ((uintptr_t) &(((type *) 0)->field))


void dummy(void) {
    return;
}

static volatile uint32_t* current_ip;
static volatile uint32_t* current_sp;
static volatile uint32_t* next_ip;
static volatile uint32_t* next_sp;

/* TODO: chenge page global directly */
void switch_context(void) {
    Process* current_p = processes[current_p_idx];
    Context* current_c = current_p->context;
    Process* next_p    = processes[next_p_idx];
    Context* next_c    = next_p->context;

    current_p_idx++;
    next_p_idx++;
    if (3 <= current_p_idx) {
        current_p_idx = 0;
    }
    if (3 <= next_p_idx) {
        next_p_idx = 0;
    }

    current_ip =&current_c->eip;
    current_sp =&current_c->esp;
    next_ip = &next_c->eip;
    next_sp = &next_c->esp;

    __asm__ volatile (
        // "pushfl                             \n\t"   // store eflags
        // "pushl %%ebp                        \n\t"
        // "pushl %%eax                        \n\t"
        // "pushl %%ecx                        \n\t"
        // "pushl %%edx                        \n\t"
        // "pushl %%ebx                        \n\t"

        // "movl  $next_turn, %[current_ip]    \n\t"   // store ip
        // "movl  %%esp, %[current_sp]         \n\t"   // store sp
        // "movl  %[next_sp], %%esp            \n\t"   // restore sp
        // "pushl %[next_ip]                   \n\t"   // restore ip
        // "jmp dummy                          \n\t"   // change context

        // "next_turn:                         \n\t"

        // "popl %%ebx                         \n\t"
        // "popl %%edx                         \n\t"
        // "popl %%ecx                         \n\t"
        // "popl %%eax                         \n\t"
        // "popl %%ebp                         \n\t"
        // "popfl                              \n\t"   // restore eflags

        // :   [current_ip] "=m" (current_c->eip),
        //     [current_sp] "=m" (current_c->esp)
        // :   [next_ip] "m" (next_c->eip),
        //     [next_sp] "m" (next_c->esp)
        // : "memory"

        ".intel_syntax noprefix \n\t"
        "pushfd                 \n\t"   // store eflags
        "push ebp               \n\t"
        "push eax               \n\t"
        "push ecx               \n\t"
        "push edx               \n\t"
        "push ebx               \n\t"

        "lea eax, next_turn     \n\t"   // store ip
        "mov ebx, [current_ip]  \n\t"   // store ip
        "mov [ebx], eax         \n\t"   // store ip

        "mov ebx, [current_sp]  \n\t"   // store ip
        "mov [ebx], esp         \n\t"   // store sp

        "mov ebx, [next_sp]     \n\t"   // restore sp
        "mov esp, [ebx]         \n\t"   // restore sp
        "mov ebx, [next_ip]     \n\t"   // restore sp
        "mov ebx, [ebx]         \n\t"   // restore sp
        "push ebx               \n\t"   // restore ip

        "jmp dummy              \n\t"   // change context

        "next_turn:             \n\t"
        "pop ebx                \n\t"
        "pop edx                \n\t"
        "pop ecx                \n\t"
        "pop eax                \n\t"
        "pop ebp                \n\t"
        "popfd                  \n\t"   // restore eflags
        :
        :
        : "memory"
    );
}


void task_a(void) {
    while (1) {
        *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 'A';
        puts("Task A\n");
        io_hlt();
    }
}


void task_b(void) {
    while (1) {
        *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 'B';
        puts("Task B\n");
        io_hlt();
    }
}


Axel_state_code init_process(void) {
    pk.context      = vmalloc(sizeof(Context));
    memset(pk.context, 0, sizeof(Context));
    pk.pid          = 0;
    pk.name         = name_k;
    __asm__ volatile (
            "movl %%esp, %[sp] \n\t"
            : [sp] "=m" (pk.stack)
            :
            : "memory"
            );

    pa.context      = vmalloc(sizeof(Context));
    memset(pa.context, 0, sizeof(Context));
    pa.context->eip = (uint32_t)(uintptr_t)task_a;
    pa.pid          = 1;
    pa.name         = name_a;
    pa.stack        = vmalloc(0x1000);
    pa.context->esp = (uint32_t)(uintptr_t)pa.stack;

    pb.context      = vmalloc(sizeof(Context));
    memset(pb.context, 0, sizeof(Context));
    pb.context->eip = (uint32_t)(uintptr_t)task_b;
    pb.pid          = 2;
    pb.name         = name_b;
    pb.stack        = vmalloc(0x1000);
    pb.context->esp = (uint32_t)(uintptr_t)pb.stack;

    processes = vmalloc(sizeof(Process*) * 3);
    processes[0] = &pk;
    processes[1] = &pa;
    processes[2] = &pb;

    return AXEL_SUCCESS;
}
