/*
 * Singly linked list
 * 2010-12-25: wuyulun
 */

#ifndef __SLIST_H__
#define __SLIST_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NSLNode {
    
    void*   item;
    
    struct _NSLNode*    next;
    
} NSLNode;

typedef struct _NSList {
    
    NSLNode*    head;
    NSLNode*    curr;

	NSLNode*	recycle;
    
    nbool		headRemoved : 1;
    
} NSList;

NSList* sll_create(void);
void sll_delete(NSList** list);
void sll_reset(NSList* list);

void sll_append(NSList* list, void* item);
void* sll_first(NSList* list);
void* sll_next(NSList* list);
void* sll_last(NSList* list);
void* sll_getAt(NSList* list, int index);
void* sll_removeFirst(NSList* list);
void* sll_removeCurr(NSList* list);

nbool sll_isEmpty(NSList* list);

#ifdef __cplusplus
}
#endif

#endif // __SLIST_H__
