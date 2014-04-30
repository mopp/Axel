/*
 * @file doubly_circularly_linked_list.c
 * @brief DoublyCircularlyLinkedList Header.
 * @author mopp
 * @version 0.2
 * @date 2014-04-25
 */

#ifndef _DOUBLY_CIRCULARLY_LINKED_LIST_H
#define _DOUBLY_CIRCULARLY_LINKED_LIST_H


#include <stdbool.h>

/*
 * free function for list node.
 * It is used in list_destruct().
 */
typedef void (*release_func)(void*);

/*
 * comparison function for list node.
 * It is used in search_list_node().
 */
typedef bool (*comp_func)(void*, void*);

/*
 * for each function for list node.
 * if return value is true, loop is abort
 */
typedef bool (*for_each_func)(void*);


/*
 * List node structure.
 * It is in List structure below.
 */
struct list_node {
    void* data;             /* pointer to stored data in node. */
    struct list_node* next; /* pointer to next position node. */
    struct list_node* prev; /* pointer to previous position node. */
};
typedef struct list_node List_node;

/* List structure */
struct list {
    List_node* node;       /* start position pointer to node.
                            * and XXX: this node is first, this node->prev is last.
                            */
    release_func free;     /* function for releasing allocated data. */
    size_t size;           /* the number of node. */
    size_t data_type_size; /* it provided by sizeof(data). */
};
typedef struct list List;


extern List* list_init(List*, size_t, release_func);
extern List_node* list_get_new_node(List*, void*);
extern List_node* list_insert_node_next(List*, List_node*, List_node*);
extern List_node* list_insert_data_next(List*, List_node*, void*);
extern List_node* list_insert_node_prev(List*, List_node*, List_node*);
extern List_node* list_insert_data_prev(List*, List_node*, void*);
extern List* list_insert_node_first(List*, List_node*);
extern List* list_insert_data_first(List*, void*);
extern List* list_insert_node_last(List*, List_node*);
extern List* list_insert_data_last(List*, void*);
extern List_node* list_remove_node(List*, List_node* target);
extern void list_delete_node(List*, List_node* target);
extern void list_destruct(List*);
extern size_t list_get_size(List const*);
extern List_node* list_for_each(List* const, for_each_func const, bool const);
extern List_node* list_node_for_each(List* const, List_node*, for_each_func const, bool const);
extern List_node* list_search_node(List*, void*);


#endif
