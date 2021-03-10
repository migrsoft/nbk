/*
 * 2010-12-25: wuyulun
 */

#include "slist.h"

static NSLNode* sll_new_node(NSList* lst)
{
	NSLNode* n;

	if (lst->recycle) {
		n = lst->recycle;
		lst->recycle = N_NULL;
		NBK_memset(n, 0, sizeof(NSLNode));
	}
	else
		n = (NSLNode*)NBK_malloc0(sizeof(NSLNode));

	return n;
}

static void sll_recycle_node(NSList* lst, NSLNode* n)
{
	if (lst->recycle)
		NBK_free(n);
	else
		lst->recycle = n;
}

NSList* sll_create(void)
{
    NSList* l = (NSList*)NBK_malloc0(sizeof(NSList));
    return l;
}

void sll_delete(NSList** lst)
{
	NSList* l = *lst;
    sll_reset(l);
	if (l->recycle)
		NBK_free(l->recycle);
    NBK_free(l);
    *lst = N_NULL;
}

void sll_reset(NSList* lst)
{
    NSLNode* p = lst->head;
    NSLNode* t;
    
    while (p) {
        t = p;
        p = p->next;
        NBK_free(t);
    }
    lst->head = lst->curr = N_NULL;
    lst->headRemoved = 0;
}

void sll_append(NSList* lst, void* item)
{
    NSLNode *n;

	n = sll_new_node(lst);
    n->item = item;
    
    if (lst->head == N_NULL) {
        lst->head = n;
        lst->curr = lst->head;
    }
    else {
        while (lst->curr->next)
            lst->curr = lst->curr->next;
        
        lst->curr->next = n;
        lst->curr = n;
    }
}

void* sll_first(NSList* lst)
{
    lst->headRemoved = 0;
    
    if (lst->head) {
        lst->curr = lst->head;
        return lst->head->item;
    }
    else
        return N_NULL;
}

void* sll_next(NSList* lst)
{
    if (lst->headRemoved) {
        lst->headRemoved = 0;
        if (lst->curr)
            return lst->curr->item;
        else
            return N_NULL;
    }
    
    if (lst->curr && lst->curr->next) {
        lst->curr = lst->curr->next;
        return lst->curr->item;
    }
    return N_NULL;
}

void* sll_last(NSList* lst)
{
    NSLNode* n = lst->curr;
    lst->headRemoved = 0;
    while (n && n->next)
        n = n->next;
    return n->item;
}

void* sll_getAt(NSList* lst, int index)
{
    int i = 0;
    NSLNode* n = lst->head;
    while (i < index && n) {
        i++;
        n = n->next;
    }
    if (n && i == index)
        return n->item;
    else
        return N_NULL;
}

void* sll_removeFirst(NSList* lst)
{
    if (lst->head) {
        NSLNode* n = lst->head;
        void* data = n->item;
        lst->head = lst->head->next;
        lst->curr = lst->head;
		sll_recycle_node(lst, n);
        return data;
    }
    else
        return N_NULL;
}

void* sll_removeCurr(NSList* lst)
{
    NSLNode* prev;
    NSLNode* t;
    void* data;
    
    if (lst->curr == N_NULL)
        return N_NULL;
    
    prev = lst->head;
    
    if (prev == lst->curr) {
        lst->head = lst->curr->next;
        if (lst->head)
            lst->head->item = lst->curr->next->item;
        lst->curr = lst->head;
        lst->headRemoved = 1;
        data = prev->item;
		sll_recycle_node(lst, prev);
        return data;
    }
    
    while (prev->next != lst->curr)
        prev = prev->next;
    
    t = lst->curr;
    data = t->item;
    
    prev->next = lst->curr->next;
    if (prev->next)
        prev->next->item = lst->curr->next->item;
    lst->curr = prev;
    lst->headRemoved = 0;

	sll_recycle_node(lst, t);
    return data;
}

nbool sll_isEmpty(NSList* lst)
{
    return ((lst->head) ? N_FALSE : N_TRUE);
}
