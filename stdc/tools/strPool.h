/*
 * strPool.h
 *
 *  Created on: 2011-1-18
 *      Author: wuyulun
 */

#ifndef STRPOOL_H_
#define STRPOOL_H_

#include "../inc/config.h"
#include "../inc/nbk_limit.h"
#include "slist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NSPCell {
    
    char    s[STR_POOL_SIZE];
    char*   p;
    
    struct _NSPCell*    next;
    
} NSPCell;

typedef struct _NStrPool {
    
    NSPCell*    head;
    NSPCell*    cur;
    
    NSList*     ls;

} NStrPool;

#ifdef NBK_MEM_TEST
int strPool_memUsed(const NStrPool* pool);
#endif

NStrPool* strPool_create(void);
void strPool_delete(NStrPool** pool);
void strPool_reset(NStrPool* pool);
char* strPool_save(NStrPool* pool, const char* str, int length);

#ifdef __cplusplus
}
#endif

#endif /* STRPOOL_H_ */
