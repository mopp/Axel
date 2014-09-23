/**
 * @file elist.h
 * @brief Equipment list
 * @author mopp
 * @version 0.1
 * @date 2014-09-23
 */

#ifndef _ELIST_H_
#define _ELIST_H_


#include <stdint.h>
#include <stdbool.h>


/* Equipment list */
typedef struct elist {
    struct elist* next;
    struct elist* prev;
} Elist;


#define elist_get_element(type, list) (type)(list)

#define elist_foreach(type, var, list) \
    for (type var = elist_get_element(type, (list)->next); (uintptr_t)var != (uintptr_t)(list); var = elist_get_element(type, ((Elist*)var)->next))


static inline Elist* elist_init(Elist* l) {
    l->next = l;
    l->prev = l;
    return l;
}


static inline Elist* elist_insert_next(Elist* l, Elist* new) {
    new->next = l->next;
    new->prev = l;
    l->next = new;
    new->next->prev = new;

    return l;
}


static inline Elist* elist_insert_prev(Elist* l, Elist* new) {
    return elist_insert_next(l->prev, new);
}


static inline Elist* elist_remove(Elist* n) {
    Elist* next = n->next;
    Elist* prev = n->prev;
    prev->next = next;
    next->prev = prev;

    return elist_init(n);
}


static inline bool elist_is_empty(Elist* n) {
    return (n->next == n->prev) && (n == n->next);
}



#endif
