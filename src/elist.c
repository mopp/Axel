/**
 * @file elist.c
 * @brief
 * @author mopp
 * @version 0.1
 * @date 2014-10-13
 */

#include <elist.h>

Elist* elist_init(Elist* l) {
    l->next = l;
    l->prev = l;
    return l;
}


Elist* elist_insert_next(Elist* l, Elist* new) {
    new->next = l->next;
    new->prev = l;
    l->next = new;
    new->next->prev = new;

    return l;
}


Elist* elist_insert_prev(Elist* l, Elist* new) {
    return elist_insert_next(l->prev, new);
}


Elist* elist_remove(Elist* n) {
    Elist* next = n->next;
    Elist* prev = n->prev;
    prev->next = next;
    next->prev = prev;

    return elist_init(n);
}


bool elist_is_empty(Elist* n) {
    return (n->next == n->prev) && (n == n->next);
}
