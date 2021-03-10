/*
 * memAlloc.h
 *
 *  Created on: 2011-7-12
 *      Author: wuyulun
 */

#ifndef MEMALLOC_H_
#define MEMALLOC_H_

#include "../inc/config.h"
#include "slist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NMemAllocChunk {
    
    uint8*  mem;
    int     used;
    
} NMemAllocChunk;

typedef struct _NMemAlloc {
    
    int     cellSize;
    int     cellsInChunk;
    
    NSList* lst;
    
} NMemAlloc;

NMemAlloc* memAlloc_create(int cellSize, int cellsInChunk);
void memAlloc_delete(NMemAlloc** mem);

void* memAlloc_alloc(NMemAlloc* mem, int size);
void memAlloc_free(NMemAlloc* mem, void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* MEMALLOC_H_ */
