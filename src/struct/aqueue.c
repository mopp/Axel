/**
 * @file array_queue.c
 * @brief queue by array.
 * @author mopp
 * @version 0.1
 * @date 2014-04-27
 */

#include <string.h>
#include <aqueue.h>
#include <stdlib.h>


Aqueue* aqueue_init(Aqueue* q, size_t t_size, size_t capacity, release_func f) {
    q->data = (void**)malloc(sizeof(void*) * capacity);
    q->first = q->last = 0;
    q->capacity = capacity;
    q->size = 0;
    q->data_type_size = t_size;
    q->free = f;

    return q;
}


bool aqueue_is_empty(Aqueue const* q) {
    return (aqueue_get_size(q) == 0) ? true : false;
}


bool aqueue_is_full(Aqueue const* q) {
    return (aqueue_get_size(q) == q->capacity) ? true : false;
}


void* aqueue_get_first(Aqueue* q) {
    return (aqueue_is_empty(q) == true) ? NULL : q->data[q->first];
}


void aqueue_delete_first(Aqueue* q) {
    if (aqueue_is_empty(q) == true || q->data[q->first] == NULL) {
        return;
    }

    ((q->free != NULL) ? q->free : free)(q->data[q->first]);
    q->data[q->first] = NULL;

    q->first = (q->first + 1 == q->capacity) ? 0 : q->first + 1;

    --q->size;
}


void* aqueue_insert(Aqueue* q, void* data) {
    if (aqueue_is_full(q) == true) {
        return NULL;
    }

    q->data[q->last] = malloc(q->data_type_size);

    memcpy(q->data[q->last], data, q->data_type_size);

    q->last = (q->last + 1 == q->capacity) ? 0 : q->last + 1;

    ++q->size;

    return data;
}


void aqueue_destruct(Aqueue* q) {
    size_t const size = aqueue_get_size(q);
    for (int i = 0; i < size; i++) {
        aqueue_delete_first(q);
    }

    free(q->data);
}


size_t aqueue_get_size(Aqueue const* q) {
    return q->size;
}


size_t aqueue_get_capacity(Aqueue const* q) {
    return q->capacity;
}
