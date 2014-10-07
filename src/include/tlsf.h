/**
 * @file tlsf.h
 * @brief Two level Segregated Fit allocater implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-04
 */


#ifndef _TLSF_H_
#define _TLSF_H_



#include <stddef.h>
#include <elist.h>
#include <macros.h>


enum {
    FL_BASE_INDEX     = 10 - 1,
    FL_MAX_INDEX      = (32 - FL_BASE_INDEX),
    SL_MAX_INDEX_LOG2 = 4,
    SL_MAX_INDEX      = PO2(SL_MAX_INDEX_LOG2),
};


struct tlsf_manager {
    Elist blocks[FL_MAX_INDEX * SL_MAX_INDEX];
    Elist pages;
    size_t total_memory_size;
    size_t free_memory_size;
    uint32_t fl_bitmap;
    uint16_t sl_bitmaps[FL_MAX_INDEX];
};
typedef struct tlsf_manager Tlsf_manager;


extern Tlsf_manager* tlsf_init(Tlsf_manager*);
extern void tlsf_destruct(Tlsf_manager*);
extern void* tlsf_malloc_align(Tlsf_manager*, size_t, size_t);
extern void* tlsf_malloc(Tlsf_manager*, size_t);
extern void tlsf_free(Tlsf_manager*, void*);



#endif
