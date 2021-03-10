// 双链表
// create by wuyulun, 2012.3.31

#ifndef __NBK_DLIST_H__
#define __NBK_DLIST_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NDLNode {

    void*   item;

    struct _NDLNode* prev;
    struct _NDLNode* next;

} NDLNode;

typedef struct _NDList {

    NDLNode*    head;
    NDLNode*    curr;

} NDList;

NDList* dll_create(void);
void dll_delete(NDList** lst);
void dll_reset(NDList* lst);

void dll_append(NDList* lst, void* item);
void dll_insertBefore(NDList* lst, void* item);
void dll_insertAfter(NDList* lst, void* item);

void* dll_first(NDList* lst);
void* dll_prev(NDList* lst);
void* dll_next(NDList* lst);

NDLNode* dll_firstNode(NDList* lst);
NDLNode* dll_prevNode(NDList* lst, NDLNode* node);
NDLNode* dll_nextNode(NDList* lst, NDLNode* node);
void* dll_getData(NDLNode* node);

#ifdef __cplusplus
}
#endif

#endif
