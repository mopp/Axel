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
    m->lost_size = 0;
    m->lost_times = 0;

    return m;
}


int memcmp(const void* buf1, const void* buf2, size_t len) {
    const unsigned char *ucb1, *ucb2, *t;
    int result = 0;
    ucb1 = buf1;
    ucb2 = buf2;
    t = buf2 + len;

    while (t != buf2 && result == 0) {
        result = *ucb1++ - *ucb2++;
    }

    return result;
}


void* memset(void* buf, const int ch, size_t len) {
    char* end = buf + len;
    char* t = buf;

    while (t < end) {
        *t++ = ch;
    }

    return buf;
}
