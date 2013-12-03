/************************************************************
 * File: src/doubly_linked_list.c
 * Description: DoublyLinkedList
 ************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_SIZE 12

struct dlinked_list_node {
    void* data;
    struct dlinked_list_node* next;
    struct dlinked_list_node* prev;
};
typedef struct dlinked_list_node* Dlinked_list_node;

Dlinked_list_node dummy = {};

/* リスト初期化 */
void init();

/* 未接続のノードを返す */
NodePointer makeOneNode(unsigned int);

/* 連結リストの先頭にキー x を持つ要素を継ぎ足す */
void insert(unsigned int);

/* キー x を持つ最初の要素を連結リストから削除する */
void delete(unsigned int);

/* リストの先頭の要素を削除する */
void deleteFirst();

/* リストの末尾の要素を削除する */
void deleteLast();

/* 先頭から順に全要素を出力 */
void printLinkedList();

/* 与えられたキーを持つノードを検索 */
NodePointer searchNodeByKey(unsigned int key);


int main(void){
    unsigned int i, cmdNum, inKey;
    char cmdStr[MAX_COMMAND_SIZE];

    init();

    scanf(" %d", &cmdNum);

    for ( i = 0; i < cmdNum; i++ ){
        scanf("%s", cmdStr);

        switch (cmdStr[0]) {
            case 'i':
                scanf("%d", &inKey);
                insert(inKey);
                break;
            case 'd':
                if (strlen(cmdStr) > 6){
                    switch (cmdStr[6]) {
                        case 'F':
                            deleteFirst();
                            break;
                        case 'L':
                            deleteLast();
                            break;
                    }
                } else {
                    scanf("%d", &inKey);
                    delete(inKey);
                }
                break;
        }
    }

    printLinkedList();

    return 0;
}


/* リスト初期化 */
void init(){
    head = makeOneNode(0);
    tail = makeOneNode(0);

    head->next = tail;
    tail->prev = head;
}


/* 未接続のノードを返す */
NodePointer makeOneNode(unsigned int key){
    NodePointer new_Node = (NodePointer)malloc(sizeof(struct node));

    new_Node->key = key;
    new_Node->next = NULL;
    new_Node->prev = NULL;

    return new_Node;
}


/* 連結リストの先頭にキー x を持つ要素を継ぎ足す */
void insert(unsigned int key){
    NodePointer new_Node, t;

    // 先頭要素退避
    t = head->next;

    // 新規ノード生成
    new_Node = makeOneNode(key);
    new_Node->next = t;
    new_Node->prev = head;
    head->next = new_Node;

    // 退避したノードを設定
    t->prev = new_Node;
}


/* キー x を持つ最初の要素を連結リストから削除する */
void delete(unsigned int key){
    NodePointer t, searched = searchNodeByKey(key);

    if (searched == NULL) {
        return;
    }

    // 再連結
    t = searched->prev;
    t->next = searched->next;
    t->next->prev = t;

    free(searched);
}


/* リストの先頭の要素を削除する */
void deleteFirst(){
    NodePointer forDelete = head->next;

    head->next = forDelete->next;
    forDelete->next->prev = head;

    free(forDelete);
}


/* リストの末尾の要素を削除する */
void deleteLast(){
    NodePointer forDelete = tail->prev;

    tail->prev = forDelete->prev;
    forDelete->prev->next = tail;

    free(forDelete);
}


/* 先頭から順に全要素を出力 */
void printLinkedList(){
    NodePointer t = head->next;

    // 空
    if (t == tail) {
        return;
    }

    while(NULL != t->next->next){
        printf("%d ", t->key);
        t = t->next;
    }
    printf("%d\n", t->key);
}


/* 与えられたキーを持つノードを検索 */
NodePointer searchNodeByKey(unsigned int key){
    /* NodePointer h = head->next, t = tail->prev; */
    NodePointer h = head->next;

    /* この問題は下記のサーチ法ではダメなようで
     *     // 先頭と末尾の双方から検索
     *     while(t != h && h != NULL && t != NULL) {
     *         if (h->key == key) {
     *             return h;
     *         }
     *
     *         if (t->key == key) {
     *             return t;
     *         }
     *
     *         h = h->next;
     *         t = t->prev;
     *     }
     */

    while (h->next != NULL) {
        if (h->key == key) {
            return h;
        }
        h = h->next;
    }

    return NULL;      // Dummy
}
