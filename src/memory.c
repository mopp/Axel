/************************************************************
 * File: graphic_txt.c
 * Description: some memory functions.
 ************************************************************/

#include <memory.h>
#include <stdint.h>
#include <stddef.h>


/* &をよく忘れるので関数で包む */
extern uint32_t LD_KERNEL_SIZE;
extern uint32_t LD_KERNEL_START;
extern uint32_t LD_KERNEL_END;


uint32_t get_kernel_start_addr(void) {
    return (uint32_t)&LD_KERNEL_START;
}


uint32_t get_kernel_end_addr(void) {
    return (uint32_t)&LD_KERNEL_END;
}


uint32_t get_kernel_size(void) {
    return (uint32_t)&LD_KERNEL_SIZE;
}

Memory_manager* get_inited_memory_manager(void) {
    Memory_manager* m = (Memory_manager*)get_kernel_end_addr();

    return m;
}

void* memset(void* buf, int ch, size_t len) {
    char* end = buf + len;
    char* t = buf;

    while (t <= end) {
        *t++ = ch;
    }

    return buf;
}
