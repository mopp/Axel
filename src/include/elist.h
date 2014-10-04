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


#define elist_derive(type, lv, ptr) \
    (type*)((uintptr_t)(ptr)-offsetof(type, lv))

#define elist_foreach(i, l, type, lv) \
    for (type* i = elist_derive(type, lv, (l)->next); (&i->lv != (l)); i = elist_derive(type, lv, i->lv.next))


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
