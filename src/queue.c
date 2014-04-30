/**
 * @file queue.c
 * @brief queue by list.
 * @author mopp
 * @version 0.1
 * @date 2014-04-24
 */

#include <stdlib.h>
#include <string.h>
#include <queue.h>

Queue* queue_init(Queue* q, size_t size, release_func f) {
    q->list = (List*)malloc(sizeof(List));

    list_init(q->list, size, f);

    return q;
}


bool queue_is_empty(Queue const* q) {
    return (list_get_size(q->list) == 0) ? true : false;
}


void* queue_get_first(Queue* q) {
    if (true == queue_is_empty(q)) {
        return NULL;
    }

    return q->list->node->data;
}


void queue_delete_first(Queue* q) {
    if (true == queue_is_empty(q)) {
        return;
    }

    list_delete_node(q->list, q->list->node);
}


void* queue_insert(Queue* q, void* data) {
    list_insert_data_last(q->list, data);

    return data;
}


void queue_destruct(Queue* q) {
    list_destruct(q->list);
    free(q->list);
}


size_t queue_get_size(Queue const* q) {
    return list_get_size(q->list);
}
