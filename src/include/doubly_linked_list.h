/************************************************************
 * File: include/doubly_linked_list.h
 * Description: DoublyLinkedList header
 ************************************************************/

#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct dlinked_list_node {
    uintptr_t data;
    struct dlinked_list_node* head;
    struct dlinked_list_node* tail;
};
typedef struct dlinked_list_node Dlinked_list_node;

extern Dlinked_list_node* const DUMMY;

extern Dlinked_list_node* init_list(Dlinked_list_node*, uintptr_t);
extern Dlinked_list_node* get_new_Dlinked_list_node(uintptr_t);
extern Dlinked_list_node* insert_head(Dlinked_list_node*, Dlinked_list_node*);
extern Dlinked_list_node* insert_tail(Dlinked_list_node*, Dlinked_list_node*);
extern void delete_node(Dlinked_list_node*);
extern Dlinked_list_node* search_node(Dlinked_list_node*, uintptr_t, bool (*)(uintptr_t, uintptr_t));


#endif
