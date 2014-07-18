/**
 * @file proc.c
 * @brief This is process function implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-07-14
 */

#include <proc.h>
#include <stdint.h>
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



static Process pa, pb;
/* static Process* current; */
static char const* name_a = "Process A";
static char const* name_b = "Process B";


void switch_context(Process* prev, Process* next) {
    /* TODO: chenge page global directly */

    /* cannot reach */
}


void task_a() {
    *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 'A';
    INF_LOOP();
}


void task_b() {
    *(uint32_t*)(KERNEL_VIRTUAL_BASE_ADDR) = 'B';
    INF_LOOP();
}


Axel_state_code init_process(void) {
    pa.context      = vmalloc(sizeof(Context));
    pa.context->eip = (uint32_t)(uintptr_t)task_a;
    pa.pid          = 1;
    pa.name         = name_a;

    pb.context      = vmalloc(sizeof(Context));
    pb.context->eip = (uint32_t)(uintptr_t)task_b;
    pb.pid          = 2;
    pb.name         = name_b;

    return AXEL_SUCCESS;
}
