#ifndef _STDLIB_H_
#define _STDLIB_H_



#include <stddef.h>
#include <paging.h>


static inline void* malloc(size_t size) {
    return vmalloc(size);
}


static inline void free(void* addr) {
    vfree(addr);
}



#endif
