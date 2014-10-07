#ifndef _STDLIB_H_
#define _STDLIB_H_



#include <stddef.h>
#include <paging.h>


static inline void* malloc(size_t size) {
    return kmalloc(size);
}


static inline void free(void* addr) {
    kfree(addr);
}



#endif
