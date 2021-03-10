/*
 * strPool.c
 *
 *  Created on: 2011-1-18
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "strPool.h"
#include "str.h"

#ifdef NBK_MEM_TEST
int strPool_memUsed(const NStrPool* pool)
{
    int size = 0;
    
    if (pool) {
        size += sizeof(NStrPool);
        
        if (pool->head) {
            NSPCell* p = pool->head;
            do {
                size += sizeof(NSPCell);
                p = p->next;
            } while (p);
        }
        
        if (pool->ls) {
            char* p = sll_first(pool->ls);
            while (p) {
                size += nbk_strlen(p) + 1;
                p = sll_next(pool->ls);
            }
        }
    }
    return size;
}
#endif

NStrPool* strPool_create(void)
{
    NStrPool* p = (NStrPool*)NBK_malloc0(sizeof(NStrPool));
    N_ASSERT(p);
    return p;
}

void strPool_delete(NStrPool** pool)
{
    NStrPool* t = *pool;    
    strPool_reset(t);
    NBK_free(t);
    *pool = N_NULL;
}

void strPool_reset(NStrPool* pool)
{
    NSPCell* c = pool->head;
    NSPCell* t;
    
    while (c) {
        t = c;
        c = c->next;
        NBK_free(t);
    }
    
    pool->head = N_NULL;
    pool->cur = N_NULL;

    if (pool->ls) {
        char* s = sll_first(pool->ls);
        while (s) {
            NBK_free(s);
            s = sll_next(pool->ls);
        }
        sll_delete(&pool->ls);
    }
}

char* strPool_save(NStrPool* pool, const char* str, int length)
{
    char* tooFar;
    char* rt;
    
    if (pool->head == N_NULL) {
        pool->head = (NSPCell*)NBK_malloc(sizeof(NSPCell));
        pool->head->p = pool->head->s;
        pool->head->next = N_NULL;
        pool->cur = pool->head;
    }
    
    tooFar = pool->cur->s + STR_POOL_SIZE;
    length = (length == -1) ? nbk_strlen(str) : length;
    if (length >= STR_POOL_SIZE) {
        char* s;
        if (pool->ls == N_NULL)
            pool->ls = sll_create();
        s = (char*)NBK_malloc(length + 1);
        nbk_strcpy(s, str);
        sll_append(pool->ls, s);
        return s;
    }
    
    if (tooFar - pool->cur->p < length + 1) {
        NSPCell* cell = (NSPCell*)NBK_malloc(sizeof(NSPCell));
        cell->p = cell->s;
        cell->next = N_NULL;
        pool->cur->next = cell;
        pool->cur = cell;
    }
    
    rt = pool->cur->p;
    nbk_strncpy(rt, str, length);
    pool->cur->p += length + 1;
    
    return rt;
}
