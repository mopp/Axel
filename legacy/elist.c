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


void elist_swap(Elist* x, Elist* y) {
    if (((x->next == y) || (y->prev == y)) || ((x->prev == y) || (y->next == x))) {
        /* Two elements are contiguous. */
        if ((x->prev == y) || (y->next == x)) {
            /* In this case, swap x and y bacause below code not supports this case. */
            Elist* tmp = x;
            x = y;
            y = tmp;
        }

        Elist* x_prev = x->prev;
        Elist* y_next = y->next;

        x->next      = y_next;
        x->prev      = y;
        y->next      = x;
        y->prev      = x_prev;
        y_next->prev = x;
        x_prev->next = y;

        return;
    }

    Elist* x_prev = x->prev;
    Elist* x_next = x->next;
    Elist* y_prev = y->prev;
    Elist* y_next = y->next;

    x_prev->next = y;
    y_prev->next = x;
    y->prev      = x_prev;
    x->prev      = y_prev;
    x_next->prev = y;
    y_next->prev = x;
    y->next      = x_next;
    x->next      = y_next;
}


bool elist_is_empty(Elist* n) {
    return (n->next == n->prev) && (n == n->next);
}
