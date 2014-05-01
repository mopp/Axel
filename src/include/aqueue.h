/**
 * @file array_queue.h
 * @brief queue by array header.
 * @author mopp
 * @version 0.1
 * @date 2014-04-27
 */

#ifndef _ARRAY_QUEUE_H_
#define _ARRAY_QUEUE_H_


#include <stddef.h>
#include <stdbool.h>
#include <atype.h>

struct aqueue {
    void** data; /* pointer to any data(void*) array. */
    size_t first, last;
    size_t capacity;
    size_t size;
    size_t data_type_size; /* it provided by sizeof(data). */
    release_func free;
};
typedef struct aqueue Aqueue;


extern Aqueue* aqueue_init(Aqueue*, size_t, size_t, release_func);
extern bool aqueue_is_empty(Aqueue const*);
extern bool aqueue_is_full(Aqueue const*);
extern void* aqueue_get_first(Aqueue*);
extern void aqueue_delete_first(Aqueue*);
extern void* aqueue_insert(Aqueue*, void*);
extern void aqueue_destruct(Aqueue*);
extern size_t aqueue_get_size(Aqueue const*);
extern size_t aqueue_get_capacity(Aqueue const*);


#endif
