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


extern Elist* elist_init(Elist* l);
extern Elist* elist_insert_next(Elist* l, Elist* new);
extern Elist* elist_insert_prev(Elist* l, Elist* new);
extern Elist* elist_remove(Elist* n);
extern bool elist_is_empty(Elist* n);



#endif
