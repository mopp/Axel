/**
 * @file aqueue.c
 * @brief Queue implemented by array.
 * @author mopp
 * @version 0.1
 * @date 2014-04-27
 */

#include <assert.h>
#include <utils.h>
#include <aqueue.h>
#include <paging.h>


Aqueue* aqueue_init(Aqueue* q, size_t type_size, size_t capacity, release_func f) {
    assert(q != NULL);

    q->data = (void**)kmalloc(sizeof(void*) * capacity);

    /* allocate all element area. */
    void* elements = kmalloc(type_size * capacity);
    uintptr_t addr = (uintptr_t)elements;

    for (size_t i = 0; i < capacity; i++) {
        q->data[i] = (void*)(addr + (i * type_size));
    }

    q->first = q->last = 0;
    q->capacity = capacity;
    q->size = 0;
    q->data_type_size = type_size;
    q->free = f;

    return q;
}


bool aqueue_is_empty(Aqueue const* q) {
    assert(q != NULL);

    return (aqueue_get_size(q) == 0) ? true : false;
}


bool aqueue_is_full(Aqueue const* q) {
    assert(q != NULL);

    return (aqueue_get_size(q) == q->capacity) ? true : false;
}


void* aqueue_get_first(Aqueue* q) {
    assert(q != NULL);

    if (true == aqueue_is_empty(q)) {
        return NULL;
    }

    return q->data[q->first];
}


void aqueue_delete_first(Aqueue* q) {
    assert(q != NULL);

    if (aqueue_is_empty(q) == true || q->data[q->first] == NULL) {
        return;
    }

    q->first = (q->first + 1 == q->capacity) ? 0 : q->first + 1;

    --q->size;
}


void* aqueue_insert(Aqueue* q, void* data) {
    if (aqueue_is_full(q) == true) {
        return NULL;
    }

    memcpy(q->data[q->last], data, q->data_type_size);

    q->last = (q->last + 1 == q->capacity) ? 0 : q->last + 1;

    ++q->size;

    return data;
}


void aqueue_destruct(Aqueue* q) {
    assert(q != NULL);

    size_t const size = aqueue_get_size(q);
    for (int i = 0; i < size; i++) {
        ((q->free != NULL) ? q->free : kfree)(q->data[i]);
    }

    kfree(q->data[0]); /* In aqueue_init, malloced addr is set at index 0 of array. */
    kfree(q->data);

    q->capacity = 0;
    q->free = 0;
}


size_t aqueue_get_size(Aqueue const* q) {
    assert(q != NULL);

    return q->size;
}


size_t aqueue_get_capacity(Aqueue const* q) {
    assert(q != NULL);

    return q->capacity;
}
