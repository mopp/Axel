/**
 * @file queue.c
 * @brief Queue First in first out structure.
 * @author mopp
 * @version 0.1
 * @date 2014-04-13
 */

#include <queue.h>
#include <stdint.h>
#include <macros.h>


Queue* init_queue(Queue* const q) {
    q->size = 0;

    // first is dummy node.
    init_list(q->first, NULL_INTPTR_T);
    q->last = q->first;

    return q;
}


bool is_empty_queue(Queue const* const q) {
    return (q->size == 0) ? true : false;
}


uintptr_t get_first(Queue* const q) {
    if (q->size <= 0) {
        return NULL_INTPTR_T;
    }

    return q->first->tail->data;
}


void remove_first(Queue* const q) {
    if (q->size <= 0) {
        return;
    }

    Dlinked_list_node* t = q->first->tail;
    q->first->tail = q->first->tail->tail;

    delete_node(t);

    --(q->size);
}


uintptr_t enqueue(Queue* const q, uintptr_t data) {
    q->last = insert_tail(q->last, get_new_dlinked_list_node(data));
    ++(q->size);

    return data;
}


uintptr_t dequeue(Queue* const q) {
    uintptr_t t = get_first(q);

    remove_first(q);

    return t;
}
