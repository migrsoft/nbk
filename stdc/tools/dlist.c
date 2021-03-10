#include "dlist.h"

NDList* dll_create(void)
{
    NDList* lst = (NDList*)NBK_malloc0(sizeof(NDList));
    return lst;
}

void dll_delete(NDList** lst)
{
    dll_reset(*lst);
    NBK_free(*lst);
    *lst = N_NULL;
}

void dll_reset(NDList* lst)
{
    NDLNode* p = lst->head;
    NDLNode* t;

    while (p) {
        t = p;
        p = p->next;
        NBK_free(t);
    }

    lst->head = lst->curr = N_NULL;
}

static NDLNode* get_last_node(NDList* lst)
{
    NDLNode* n = lst->head;
    while (n && n->next)
        n = n->next;
    return n;
}

void dll_append(NDList* lst, void* item)
{
    NDLNode* n = (NDLNode*)NBK_malloc0(sizeof(NDLNode));
    n->item = item;

    if (lst->head == N_NULL) {
        lst->head = n;
        lst->curr = n;
    }
    else {
        if (lst->curr == N_NULL)
            lst->curr = get_last_node(lst);

        lst->curr->next = n;
        n->prev = lst->curr;
        lst->curr = n;
    }
}

void dll_insertBefore(NDList* lst, void* item)
{
    NDLNode* n = (NDLNode*)NBK_malloc0(sizeof(NDLNode));
    n->item = item;

    N_ASSERT(lst->curr);

    if (lst->head == lst->curr) {
        n->next = lst->curr;
        lst->curr->prev = n;
        lst->head = n;
        lst->curr = n;
    }
    else {
        n->prev = lst->curr->prev;
        n->next = lst->curr;
        lst->curr->prev->next = n;
        lst->curr->prev = n;
        lst->curr = n;
    }
}

void dll_insertAfter(NDList* lst, void* item)
{
    NDLNode* n = (NDLNode*)NBK_malloc0(sizeof(NDLNode));
    n->item = item;

    if (lst->curr == N_NULL)
        lst->curr = get_last_node(lst);

    if (lst->curr == N_NULL) { // list is empty
        lst->head = n;
        lst->curr = n;
    }
    else {
        n->prev = lst->curr;
        n->next = lst->curr->next;
        if (lst->curr->next)
            lst->curr->next->prev = n;
        lst->curr->next = n;
        lst->curr = n;
    }
}

void* dll_first(NDList* lst)
{
    if (lst->head) {
        lst->curr = lst->head;
        return lst->curr->item;
    }
    return N_NULL;
}

void* dll_prev(NDList* lst)
{
    if (lst->curr && lst->curr->prev) {
        lst->curr = lst->curr->prev;
        return lst->curr->item;
    }
    return N_NULL;
}

void* dll_next(NDList* lst)
{
    if (lst->curr && lst->curr->next) {
        lst->curr = lst->curr->next;
        return lst->curr->item;
    }
    return N_NULL;
}

NDLNode* dll_firstNode(NDList* lst)
{
    lst->curr = lst->head;
    return lst->head;
}

NDLNode* dll_prevNode(NDList* lst, NDLNode* node)
{
    lst->curr = node->prev;
    return node->prev;
}

NDLNode* dll_nextNode(NDList* lst, NDLNode* node)
{
    lst->curr = node->next;
    return node->next;
}

void* dll_getData(NDLNode* node)
{
    return node->item;
}
