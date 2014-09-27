/*
 * @file struct/dlist.c
 * @brief This list is DoublyCircularlyLinkedList.
 *      node = X0, X1
 *      prev                                next
 *          ... <-> X0 <-> X1 <-> X0 <-> X1 <-> ...
 * @author mopp
 * @version 0.2
 * @date 2014-04-25
 */

#include <assert.h>
#include <utils.h>
#include <stdlib.h>
#include <dlist.h>


/**
 * @brief List default free function.
 * @param l This is not used.
 * @param d Pointer to any object is freed.
 */
static void default_free(Dlist* l, void* d) {
    free(d);
}


/**
 * @brief Initialize list.
 * @param l Pointer to list.
 * @param size Size of stored data type in list.
 * @param f Pointer to function for release data in list.
 * @return Pointer to list.
 */
Dlist* dlist_init(Dlist* l, size_t size, dlist_release_func f) {
    l->node = NULL;
    l->free = (f == NULL) ? (default_free) : (f);
    l->size = 0;
    l->data_type_size = size;

    return l;
}


/**
 * @brief Allocate new node and set data in it.
 * @param l Pointer to list.
 * @param data Pointer to set data into new node.
 * @return Pointer to new node.
 */
Dlist_node* dlist_get_new_node(Dlist* l, void* data) {
    Dlist_node* n = (Dlist_node*)malloc(sizeof(Dlist_node));
    if (n == NULL) {
        return NULL;
    }

    n->next = n->prev = NULL;

    /* Copy data to new area */
    n->data = malloc(l->data_type_size);
    if (n->data == NULL) {
        return NULL;
    }
    memcpy(n->data, data, l->data_type_size);

    return n;
}


/**
 * @brief Add new node into next of second argument node.
 *        After execution, list becames "... -> target -> new -> ...".
 * @param l Pointer to list.
 * @param target Pointer to base list.
 * @param new Pointer to added node.
 * @return Pointer to added node(third argument).
*/
Dlist_node* dlist_insert_node_next(Dlist* l, Dlist_node* target, Dlist_node* new) {
    assert(l != NULL && target != NULL);

    new->next = target->next;
    new->next->prev = new;

    new->prev = target;
    new->prev->next = new;

    ++(l->size);

    return new;
}


/**
 * @brief Add new data into next of argument node.
 *        And allocate new node for data of argument.
 *        After execution, list is "... -> target -> new(has data) -> ...".
 * @param l Pointer to list.
 * @param target Pointer to base list.
 * @param data Pointer to added data.
 * @return Pointer to allocated node.
 */
Dlist_node* dlist_insert_data_next(Dlist* l, Dlist_node* target, void* data) {
    assert(l != NULL && target != NULL);

    return dlist_insert_node_next(l, target, dlist_get_new_node(l, data));
}


/**
 * @brief Add new node into previous of argument node.
 *        After execution, list is "... -> new -> target -> ...".
 * @param l Pointer to list.
 * @param target Pointer to base node.
 * @param new Pointer to added node.
 * @return Pointer to added node(third argument).
 */
Dlist_node* dlist_insert_node_prev(Dlist* l, Dlist_node* target, Dlist_node* new) {
    assert(l != NULL && target != NULL);

    new->prev = target->prev;
    new->prev->next = new;

    new->next = target;
    target->prev = new;

    ++(l->size);

    return new;
}


/**
 * @brief Add new data into prev of argument node.
 *        And allocate new node for data of argument.
 *        After execution, list is "... -> new(has data) -> target -> ...".
 * @param l Pointer to list.
 * @param target Pointer to base node.
 * @param data Pointer to set data into new node.
 * @return Pointer to allocated node.
 */
Dlist_node* dlist_insert_data_prev(Dlist* l, Dlist_node* target, void* data) {
    assert(l != NULL && target != NULL);

    return dlist_insert_node_prev(l, target, dlist_get_new_node(l, data));
}


/**
 * @brief Insert node when list has NOT any node.
 * @param l Pointer to list.
 * @param new Pointer to added node.
 */
static inline void dlist_insert_first_node(Dlist* l, Dlist_node* new) {
    assert(l->node == NULL);

    /* Set pointer to self. */
    l->node = new;
    l->node->next = l->node->prev = l->node;

    ++l->size;

    assert(l->node != NULL);
}


/**
 * @brief Add new node at first in argument list.
 * @param l Pointer to list.
 * @param new Pointer to added node.
 * @return Pointer to list.
 */
Dlist* dlist_insert_node_first(Dlist* l, Dlist_node* new) {
    if (l->node == NULL) {
        dlist_insert_first_node(l, new);
    } else {
        l->node = dlist_insert_node_prev(l, l->node, new);
    }

    return l;
}


/**
 * @brief Add new data into first position in argument list.
 * @param l Pointer to list.
 * @param data Pointer to added data.
 * @return Pointer to list.
 */
Dlist* dlist_insert_data_first(Dlist* l, void* data) {
    Dlist_node* n = dlist_get_new_node(l, data);
    return (n == NULL) ? NULL : dlist_insert_node_first(l, n);
}



/**
 * @brief Add new node into last positio in argument list.
 * @param l Pointer to list.
 * @param new Pointer to added node.
 * @return Pointer to list.
 */
Dlist* dlist_insert_node_last(Dlist* l, Dlist_node* new) {
    if (l->node == NULL) {
        dlist_insert_first_node(l, new);
    } else {
        l->node->prev = dlist_insert_node_next(l, l->node->prev, new);
    }

    return l;
}


/**
 * @brief Add new node after last node in list
 * @param l Pointer to list.
 * @param data Pointer to adde data.
 * @return Pointer to list.
 */
Dlist* dlist_insert_data_last(Dlist* l, void* data) {
    Dlist_node* n = dlist_get_new_node(l, data);
    return (n == NULL) ? NULL : dlist_insert_node_last(l, n);
}



/**
 * @brief Remove argument node in list.
 *        And this NOT releases data.
 *        Therefor, You MUST release data yourself.
 * @param l Pointer to list.
 * @param target Pointer to deleted node.
 * @return Pointer to removed node.
 */
Dlist_node* dlist_remove_node(Dlist* l, Dlist_node* target) {
    assert(target != NULL);

    if (l->size == 1) {
        /*
         * Size equals 1.
         * This means that list has only "node".
         */
        l->node = NULL;
    } else if (target == l->node) {
        /* change "node". */
        l->node->prev->next = target->next;
        target->next->prev = l->node->prev;
        l->node = target->next;
    } else {
        target->next->prev = target->prev;
        target->prev->next = target->next;
    }

    --l->size;

    return target;
}


/**
 * @brief Delete argument node in list.
 *        And this releases data.
 * @param l Pointer to list.
 * @param target Pointer to deleted node.
 */
void dlist_delete_node(Dlist* l, Dlist_node* target) {
    assert(target != NULL);

    Dlist_node* n = dlist_remove_node(l, target);
    l->free(l, n->data);
    free(n);
}


/**
 * @brief All node in list be freed.
 * @param l Pointer to list.
 */
void dlist_destruct(Dlist* l) {
    if (l->node == NULL) {
        /* Do nothing. */
        return;
    }

    Dlist_node* n = l->node;
    Dlist_node* t = l->node;
    Dlist_node* limit = l->node->prev;

    if (l->size != 1) {
        do {
            t = n->next;
            l->free(l, n->data);
            free(n);
            n = t;
        } while (n != limit);
    }
    l->free(l, t->data);
    free(t);

    l->node = NULL;
    l->size = 0;
}


/**
 * @brief Get the number of node in list.
 * @param l Pointer to list.
 * @return The number of node.
 */
size_t dlist_get_size(Dlist const* l) {
    return l->size;
}


/**
 * @brief This function provide for_each loop based on argument node.
 * @param l Pointer to list.
 * @param f Pointer to function witch decide stop or continue loop.
 * @param is_reverse Loop direction flag. if it is true, loop is first to last.
 * @return Pointer to node when loop stoped or NULL.
 */
Dlist_node* dlist_node_for_each(Dlist* const l, Dlist_node* n, dlist_for_each_func const f, bool const is_reverse) {
    assert(f != NULL);

    if (l->node == NULL) {
        return NULL;
    }

    if (l->node == l->node->next) {
        /* List has only one node. */
        f(l, l->node->data);
        return l->node;
    }

    Dlist_node const* const stored = n;
    do {
        if (true == f(l, n->data)) {
            return n;
        }
        n = (true == is_reverse) ? n->prev : n->next;
    } while (n != stored);

    return NULL;
}


/**
 * @brief This function provide for_each loop.
 * @param l Pointer to list.
 * @param f Pointer to function witch decide stop or continue loop.
 * @param is_reverse Loop direction flag. if it is true, loop is first to last.
 * @return Pointer to node when loop stoped or NULL.
 */
Dlist_node* dlist_for_each(Dlist* const l, dlist_for_each_func const f, bool const is_reverse) {
    assert(f != NULL);
    return dlist_node_for_each(l, ((true == is_reverse) ? l->node->prev : l->node), f, is_reverse);
}


/* Searchable_list extends List for searching. */
typedef struct {
    Dlist list;
    void* target;
} Searchable_list;


static bool search_loop(Dlist* l, void* data) {
    void* target = ((Searchable_list const*)l)->target;
    return (data == target || 0 == memcmp(target, data, l->data_type_size)) ? true : false;
}


/**
 * @brief Search node witch has argument data.
 * @param l Pointer to list.
 * @param data Pointer to search key data.
 * @return Pointer to searched node.
 */
Dlist_node* dlist_search_node(Dlist* l, void* data) {
    if (l->node == NULL) {
        return NULL;
    }

    Searchable_list sl = {.list = *l, .target = data};

    Dlist_node* n = dlist_for_each(&(sl.list), search_loop, false);

    return (n == NULL) ? NULL : n;
}


/**
 * @brief Exchange each data.
 * @param x Pointer to list_node.
 * @param y Pointer to list_node.
 */
void dlist_swap_data(Dlist_node* x, Dlist_node* y) {
    void* t = x->data;
    x->data = y->data;
    y->data = t;
}
