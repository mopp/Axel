/************************************************************
 * File: src/doubly_linked_list.c
 * Description: DoublyLinkedList
 ************************************************************/
/* #define DEBUG */

#ifdef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#endif

#include <doubly_linked_list.h>
#include <memory.h>


/* 終端子 */
Dlinked_list_node * const DUMMY = &(Dlinked_list_node){.data = 0, .head = NULL, .tail = NULL};


/* 新しいノードを確保しデータを設定 */
Dlinked_list_node* get_new_Dlinked_list_node(uintptr_t data) {
    Dlinked_list_node* n = (Dlinked_list_node*)malloc(sizeof(Dlinked_list_node));

    n->data = data;
    n->head = DUMMY;
    n->tail = DUMMY;

    return n;
}


/* ダミーとデータを付加してリストを初期化 */
Dlinked_list_node* init_list(Dlinked_list_node* dl, uintptr_t data) {
    dl->data = data;
    dl->head = DUMMY;
    dl->tail = DUMMY;

    return dl;
}


/*
 * リストの先頭にノードを挿入
 * @return 新しい先頭
 */
Dlinked_list_node* insert_head(Dlinked_list_node* target, Dlinked_list_node* added) {
    added->head = target->head;
    added->head->tail = added;

    added->tail = target;
    target->head = added;

    return added;
}


/*
 * リストの末尾にノードを挿入
 * @return 新しい末尾
 */
Dlinked_list_node* insert_tail(Dlinked_list_node* target, Dlinked_list_node* added) {
    added->tail = target->tail;
    added->tail->head = added;

    added->head = target;
    target->tail = added;

    return added;
}


/* 渡されたリストの削除 */
void delete_node(Dlinked_list_node* target) {
    target->head->tail = target->tail;
    target->tail->head = target->head;

    free(target);
}


/* ノードを検索 */
Dlinked_list_node* search_node(Dlinked_list_node* dl, uintptr_t data, bool (*comp_func)(uintptr_t,  uintptr_t)) {
    if (dl->head == NULL || dl->tail == NULL) {
        return DUMMY;
    }

    while (dl->head != DUMMY) {
        dl = dl->head;
    }

    while (comp_func(dl->data, data) == false) {
        dl = dl->tail;

        if (dl == DUMMY) {
            return DUMMY;
        }
    }

    return dl;
}


#ifdef DEBUG

#define SIZE_OF_ADD 3

typedef struct {
    Dlinked_list_node* dummy_addr;
} test_struct;


void print_one(Dlinked_list_node* dl) {
    printf("-----\n");
    printf("addr: %p\n", dl);
    printf("head: %p\n", dl->head);
    printf("tail: %p\n", dl->tail);
    if (dl != DUMMY) {
        printf("data: %p\n", ((test_struct*)(dl->data))->dummy_addr);
    }
    printf("-----\n");
}


void print_all(Dlinked_list_node* dl) {
    while (dl->head != DUMMY) {
        dl = dl->head;
    }
    dl = dl->tail;

    printf("head\n");

    while (dl != DUMMY) {
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
    test_struct ts = {.dummy_addr = DUMMY};
    Dlinked_list_node* t = get_new_Dlinked_list_node((uintptr_t)&ts);

    /* printf("DUMMY\n"); */
    /* print_one(DUMMY); */

    init_list(t, (uintptr_t)&ts);

    /* printf("First Node\n"); */
    /* print_one(t); */
    assert(DUMMY == t->head);
    assert(DUMMY == t->tail);

    Dlinked_list_node* tt = t;
    for (int i = 0; i < SIZE_OF_ADD; i++) {
        Dlinked_list_node* target = tt;
        Dlinked_list_node* head = tt->head;
        Dlinked_list_node* tail = tt->tail;

        tt = insert_tail(tt, get_new_Dlinked_list_node((uintptr_t)&ts));

        assert(target == tt->head);
        assert(head == tt->head->head);
        assert(tail == tt->tail);
    }

    for (int i = 0; i < SIZE_OF_ADD; i++) {
        t = insert_head(t, get_new_Dlinked_list_node((uintptr_t)&ts));
        assert(DUMMY == t->head);
    }

    tt = t->tail;
    for (int i = 0; i < SIZE_OF_ADD; i++) {
        delete_node(t);
        assert(DUMMY == tt->head);

        t = tt;
        tt = t->tail;
    }

    /* printf("\nPrint All Node\n\n"); */
    /* print_all(t); */

    Dlinked_list_node* nt = get_new_Dlinked_list_node((uintptr_t)&ts);
    test_struct tn[10];
    for (int i = 0; i < 10; i++) {
        Dlinked_list_node* nn = get_new_Dlinked_list_node((uintptr_t)(tn + i));
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

#endif
