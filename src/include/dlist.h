/*
 * @file include/dlist.h
 * @brief DoublyCircularlyLinkedList Header.
 * @author mopp
 * @version 0.2
 * @date 2014-04-25
 */

#ifndef _DLIST_H_
#define _DLIST_H_


#include <stdbool.h>
#include <stddef.h>


struct dlist;

/*
 * for each function for list node.
 * if return value is true, loop is abort
 */
typedef bool (*dlist_for_each_func)(struct dlist*, void*);

/*
 * free function for list node.
 * It is used in list_destruct().
 */
typedef void (*dlist_release_func)(struct dlist*, void*);

/*
 * comparison function for list node.
 * It is used in search_list_node().
 */
typedef bool (*dlist_comp_func)(struct dlist*, void*, void*);


/*
 * List node structure.
 * It is in List structure below.
 */
struct dlist_node {
    void* data;              /* pointer to stored data in node. */
    struct dlist_node* next; /* pointer to next position node. */
    struct dlist_node* prev; /* pointer to previous position node. */
};
typedef struct dlist_node Dlist_node;

/* List structure */
struct dlist {
    Dlist_node* node;      /* start position pointer to node.
                           * and XXX: this node is first, this node->prev is last.
                           */
    dlist_release_func free;     /* function for releasing allocated data. */
    size_t size;           /* the number of node. */
    size_t data_type_size; /* it provided by sizeof(data). */
};
typedef struct dlist Dlist;


extern Dlist* dlist_init(Dlist*, size_t, dlist_release_func);
extern Dlist_node* dlist_get_new_node(Dlist*, void*);
extern Dlist_node* dlist_insert_node_next(Dlist*, Dlist_node*, Dlist_node*);
extern Dlist_node* dlist_insert_data_next(Dlist*, Dlist_node*, void*);
extern Dlist_node* dlist_insert_node_prev(Dlist*, Dlist_node*, Dlist_node*);
extern Dlist_node* dlist_insert_data_prev(Dlist*, Dlist_node*, void*);
extern Dlist* dlist_insert_node_first(Dlist*, Dlist_node*);
extern Dlist* dlist_insert_data_first(Dlist*, void*);
extern Dlist* dlist_insert_node_last(Dlist*, Dlist_node*);
extern Dlist* dlist_insert_data_last(Dlist*, void*);
extern Dlist_node* dlist_remove_node(Dlist*, Dlist_node* target);
extern void dlist_delete_node(Dlist*, Dlist_node* target);
extern void dlist_destruct(Dlist*);
extern size_t dlist_get_size(Dlist const*);
extern Dlist_node* dlist_for_each(Dlist* const, dlist_for_each_func const, bool const);
extern Dlist_node* dlist_node_for_each(Dlist* const, Dlist_node*, dlist_for_each_func const, bool const);
extern Dlist_node* dlist_search_node(Dlist*, void*);
extern void dlist_swap_data(Dlist_node*, Dlist_node*);


#define dlist_get_data(type, node_p) (*(type*)((node_p)->data))
#define dlist_cast_data(type, d) (*(type*)(d))



#endif
