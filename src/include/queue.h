/**
 * @file queue.c
 * @brief queue header.
 * @author mopp
 * @version 0.1
 * @date 2014-04-24
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_


#include <list.h>

struct queue {
    List* list;
};
typedef struct queue Queue;


extern Queue* queue_init(Queue*, size_t, release_func);
extern bool queue_is_empty(Queue const*);
extern void* queue_get_first(Queue*);
extern void queue_delete_first(Queue*);
extern void* queue_insert(Queue*, void*);
extern void queue_destruct(Queue*);
extern size_t queue_get_size(Queue const*);


#endif
