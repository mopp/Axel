/**
 * @file queue.c
 * @brief  Queue structure header.
 * @author mopp
 * @version 0.1
 * @date 2014-04-13
 */


#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <doubly_linked_list.h>

struct queue {
    Dlinked_list_node* first; /* Pointer to first element. */
    Dlinked_list_node* last;  /* Pointer to last element. */
    size_t size;              /* All element size. */
};
typedef struct queue Queue;


extern Queue* init_queue(Queue* const);
extern bool is_empty_queue(Queue const* const);
extern uintptr_t get_first(Queue* const);
extern void remove_first(Queue* const);
extern uintptr_t enqueue(Queue* const, uintptr_t);
extern uintptr_t dequeue(Queue* const);


#endif
