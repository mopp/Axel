/************************************************************
 * File: src/doubly_linked_list.c
 * Description: DoublyLinkedList
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <doubly_linked_list.h>

#define SIZE_OF_ADD 3

typedef struct {
    Dlinked_list_node* dummy_addr;
} test_struct;


void print_one(Dlinked_list_node* dl) {
    printf("-----\n");
    printf("addr: %p\n", dl);
    printf("head: %p\n", dl->head);
    printf("tail: %p\n", dl->tail);
    if (dl != DUMMY_NODE) {
        printf("data: %p\n", ((test_struct*)(dl->data))->dummy_addr);
    }
    printf("-----\n");
}


void print_all(Dlinked_list_node* dl) {
    while (dl->head != DUMMY_NODE) {
        dl = dl->head;
    }
    dl = dl->tail;

    printf("head\n");

    while (dl != DUMMY_NODE) {
        print_one(dl);

        dl = dl->tail;
    }
    printf("tail\n");
}

bool comp(uintptr_t x, uintptr_t y) {
    if (((test_struct*)x)->dummy_addr == ((test_struct*)y)->dummy_addr) {
        return true;
    }
    return false;
}

int main(void) {
    test_struct ts = {.dummy_addr = DUMMY_NODE};
    Dlinked_list_node* t = get_new_dlinked_list_node((uintptr_t)&ts);

    /* printf("DUMMY_NODE\n"); */
    /* print_one(DUMMY_NODE); */

    init_list(t, (uintptr_t)&ts);

    /* printf("First Node\n"); */
    /* print_one(t); */
    assert(DUMMY_NODE == t->head);
    assert(DUMMY_NODE == t->tail);

    Dlinked_list_node* tt = t;
    for (int i = 0; i < SIZE_OF_ADD; i++) {
        Dlinked_list_node* target = tt;
        Dlinked_list_node* head = tt->head;
        Dlinked_list_node* tail = tt->tail;

        tt = insert_tail(tt, get_new_dlinked_list_node((uintptr_t)&ts));

        assert(target == tt->head);
        assert(head == tt->head->head);
        assert(tail == tt->tail);
    }

    for (int i = 0; i < SIZE_OF_ADD; i++) {
        t = insert_head(t, get_new_dlinked_list_node((uintptr_t)&ts));
        assert(DUMMY_NODE == t->head);
    }

    tt = t->tail;
    for (int i = 0; i < SIZE_OF_ADD; i++) {
        delete_node(t);
        assert(DUMMY_NODE == tt->head);

        t = tt;
        tt = t->tail;
    }

    /* printf("\nPrint All Node\n\n"); */
    /* print_all(t); */

    Dlinked_list_node* nt = get_new_dlinked_list_node((uintptr_t)&ts);
    test_struct tn[10];
    for (int i = 0; i < 10; i++) {
        Dlinked_list_node* nn = get_new_dlinked_list_node((uintptr_t)(tn + i));
        insert_tail(nt, nn);
        tn[i].dummy_addr = nn;
    }
    /* printf("\n Searched Node\n\n"); */
    /* print_one(search_node(nt, &ts, &comp)); */
    assert(search_node(nt, (uintptr_t)&ts, &comp) == nt);
    /* print_all(nt); */
    printf("Test is Passed\n");

    return 0;
}

