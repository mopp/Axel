/*
 * @file doubly_circularly_linked_list.c
 * @brief This list is DoublyCircularlyLinkedList.
 *      prev                            next
 *          node(X0)<->X1<->node(X0)->X1->...
 * @author mopp
 * @version 0.2
 * @date 2014-04-25
 */
/* #define NDBUG */

#include <string.h>
#include <stdlib.h>
#include <list.h>


/**
 * @brief initialize list.
 * @param l pointer to list.
 * @param size size of stored data type in list.
 * @param f pointer to function for release data in list.
 * @return pointer to list.
 */
List* list_init(List* l, size_t size, release_func f) {
    l->node = NULL;
    l->free = f;
    l->size = 0;
    l->data_type_size = size;

    return l;
}


/**
 * @brief allocate new node and set data in it.
 * @param l pointer to list.
 * @param data pointer to set data into new node.
 * @return pointer to new node.
 */
List_node* list_get_new_node(List* l, void* data) {
    List_node* n = (List_node*)malloc(sizeof(List_node));
    if (n == NULL) {
        return NULL;
    }

    n->next = n->prev = NULL;

    /* copy data to new area */
    n->data = malloc(l->data_type_size);
    if (n->data == NULL) {
        return NULL;
    }
    memcpy(n->data, data, l->data_type_size);

    return n;
}


/**
 * @brief add new node into next of second argument node.
 *        after execute, list became "... -> target -> new -> ...".
 * @param l pointer to list.
 * @param target pointer to base list.
 * @param new pointer to added node.
 * @return pointer to added node(third argument).
 */
List_node* list_insert_node_next(List* l, List_node* target, List_node* new) {
    new->next = target->next;
    new->next->prev = new;

    new->prev = target;
    new->prev->next = new;

    ++(l->size);

    return new;
}


/**
 * @brief add new data into next of argument node.
 *        And allocate new node for data of argument.
 *        after execute, list is "... -> target -> new(has data) -> ...".
 * @param l pointer to list.
 * @param target pointer to base list.
 * @param data pointer to added data.
 * @return pointer to allocated node.
 */
List_node* list_insert_data_next(List* l, List_node* target, void* data) {
    return list_insert_node_next(l, target, list_get_new_node(l, data));
}


/**
 * @brief add new node into previous of argument node.
 *        after execute, list is "... -> new -> target -> ...".
 * @param l pointer to list.
 * @param target pointer to base node.
 * @param new pointer to added node.
 * @return pointer to added node(third argument).
 */
List_node* list_insert_node_prev(List* l, List_node* target, List_node* new) {
    new->prev = target->prev;
    new->prev->next = new;

    new->next = target;
    target->prev = new;

    ++(l->size);

    return new;
}


/**
 * @brief add new data into prev of argument node.
 *        And allocate new node for data of argument.
 *        after execute, list is "... -> new(has data) -> target -> ...".
 * @param l pointer to list.
 * @param target pointer to base node.
 * @param data pointer to set data into new node.
 * @return pointer to allocated node.
 */
List_node* list_insert_data_prev(List* l, List_node* target, void* data) {
    return list_insert_node_prev(l, target, list_get_new_node(l, data));
}


/**
 * @brief insert node when list has NOT any node.
 * @param l pointer to list.
 * @param new pointer to added node.
 */
static inline void list_insert_first_node(List* l, List_node* new) {
    /* set pointer to self. */
    l->node = new;
    l->node->next = l->node->prev = l->node;

    ++l->size;
}


/**
 * @brief add new node at first in argument list.
 * @param l pointer to list.
 * @param new pointer to added node.
 * @return pointer to list.
 */
List* list_insert_node_first(List* l, List_node* new) {
    if (l->node == NULL) {
        list_insert_first_node(l, new);
    } else {
        l->node = list_insert_node_prev(l, l->node, new);
    }

    return l;
}


/**
 * @brief add new data into first position in argument list.
 * @param l pointer to list.
 * @param data pointer to added data.
 * @return pointer to list.
 */
List* list_insert_data_first(List* l, void* data) {
    List_node* n = list_get_new_node(l, data);
    return (n == NULL) ? NULL : list_insert_node_first(l, n);
}



/**
 * @brief add new node into last positio in argument list.
 * @param l pointer to list.
 * @param new pointer to added node.
 * @return pointer to list.
 */
List* list_insert_node_last(List* l, List_node* new) {
    if (l->node == NULL) {
        list_insert_first_node(l, new);
    } else {
        l->node->prev = list_insert_node_next(l, l->node->prev, new);
    }

    return l;
}


/**
 * @brief add new node after last node in list
 * @param l pointer to list.
 * @param data pointer to adde data.
 * @return pointer to list.
 */
List* list_insert_data_last(List* l, void* data) {
    List_node* n = list_get_new_node(l, data);
    return (n == NULL) ? NULL : list_insert_node_last(l, n);
}



/**
 * @brief remove argument node in list.
 *        And this NOT releases data.
 *        therefor, You MUST release data yourself.
 * @param l pointer to list.
 * @param target pointer to deleted node.
 * @return pointer to removed node.
 */
List_node* list_remove_node(List* l, List_node* target) {
    if (l->size == 1) {
        /*
         * size equals 1.
         * this means that list has only "node".
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
 * @brief delete argument node in list.
 *        And this releases data.
 * @param l pointer to list.
 * @param target pointer to deleted node.
 */
void list_delete_node(List* l, List_node* target) {
    List_node* n = list_remove_node(l, target);
    ((l->free == NULL) ? (free) : (l->free))(n->data);
    free(n);
}


/**
 * @brief all node in list be freed.
 * @param l pointer to list.
 */
void list_destruct(List* l) {
    if (l->node == NULL) {
        /* do nothing. */
        return;
    }

    /* select default free function or user free function. */
    release_func f = (l->free != NULL) ? l->free : free;

    List_node* n = l->node;
    List_node* t = l->node;
    List_node* limit = l->node->prev;

    if (l->size != 1) {
        do {
            t = n->next;
            f(n->data);
            free(n);
            n = t;
        } while (n != limit);
    }
    f(t->data);
    free(t);

    l->node = NULL;
    l->size = 0;
}


/**
 * @brief get the number of node in list.
 * @param l pointer to list.
 * @return the number of node.
 */
size_t list_get_size(List const* l) {
    return l->size;
}


/**
 * @brief this function provide for_each loop based on argument node.
 * @param l pointer to list.
 * @param f pointer to function witch decide stop or continue loop.
 * @param is_reverse loop direction flag. if it is true, loop is first to last.
 * @return pointer to node when loop stoped or NULL.
 */
List_node* list_node_for_each(List* const l, List_node* n, for_each_func const f, bool const is_reverse) {
    if (l->node == NULL) {
        return NULL;
    }

    if (l->node == l->node->next) {
        /* when list has only one node. */
        f(l->node->data);
        return l->node;
    }

    /* List_node* n = (true == is_reverse) ? l->node->prev : l->node->next; */
    List_node const* const stored = n;
    do {
        if (true == f(n->data)) {
            return n;
        }
        n = (true == is_reverse) ? n->prev : n->next;
    } while (n != stored);

    return NULL;
}


/**
 * @brief this function provide for_each loop.
 * @param l pointer to list.
 * @param f pointer to function witch decide stop or continue loop.
 * @param is_reverse loop direction flag. if it is true, loop is first to last.
 * @return pointer to node when loop stoped or NULL.
 */
List_node* list_for_each(List* const l, for_each_func const f, bool const is_reverse) {
    return list_node_for_each(l, ((true == is_reverse) ? l->node->prev : l->node), f, is_reverse);
}


static void* search_target;
static List* search_list;
static bool search_loop(void* data) {
    return (data == search_target || 0 == memcmp(search_target, data, search_list->data_type_size)) ? true : false;
}


/**
 * @brief search node witch has argument data.
 * @param l pointer to list.
 * @param data pointer to search key data.
 * @return
 */
List_node* list_search_node(List* l, void* data) {
    if (l->node == NULL) {
        return NULL;
    }

    search_target = data;
    search_list = l;

    List_node* n = list_for_each(l, search_loop, false);

    search_target = NULL;
    search_list = NULL;

    return (n == NULL) ? NULL : n;
}
