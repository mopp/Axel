/************************************************************
 * File: src/doubly_linked_list.c
 * Description: DoublyLinkedList
 ************************************************************/

#include <doubly_linked_list.h>
#include <memory.h>


/* 終端子 */
Dlinked_list_node * const DUMMY_NODE = &(Dlinked_list_node){.data = 0, .head = NULL, .tail = NULL};


/* 新しいノードを確保しデータを設定 */
Dlinked_list_node* get_new_dlinked_list_node(uintptr_t data) {
    Dlinked_list_node* n = (Dlinked_list_node*)malloc(sizeof(Dlinked_list_node));

    n->data = data;
    n->head = DUMMY_NODE;
    n->tail = DUMMY_NODE;

    return n;
}


/* ダミーとデータを付加してリストを初期化 */
Dlinked_list_node* init_list(Dlinked_list_node* dl, uintptr_t data) {
    dl->data = data;
    dl->head = DUMMY_NODE;
    dl->tail = DUMMY_NODE;

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
        return DUMMY_NODE;
    }

    while (dl->head != DUMMY_NODE) {
        dl = dl->head;
    }

    while (comp_func(dl->data, data) == false) {
        dl = dl->tail;

        if (dl == DUMMY_NODE) {
            return DUMMY_NODE;
        }
    }

    return dl;
}
