/*
 * memAlloc.c
 *
 *  Created on: 2011-7-12
 *      Author: wuyulun
 */

#include "memAlloc.h"

NMemAlloc* memAlloc_create(int cellSize, int cellsInChunk)
{
    NMemAlloc* ma = (NMemAlloc*)NBK_malloc0(sizeof(NMemAlloc));
    ma->cellSize = cellSize;
    ma->cellsInChunk = cellsInChunk;
    return ma;
}

void memAlloc_delete(NMemAlloc** mem)
{
    NMemAlloc* ma = *mem;
    
    if (ma->lst) {
        NMemAllocChunk* chunk = (NMemAllocChunk*)sll_first(ma->lst);
        while (chunk) {
            NBK_free(chunk->mem);
            NBK_free(chunk);
            chunk = (NMemAllocChunk*)sll_next(ma->lst);
        }
        sll_delete(&ma->lst);
    }
    
    NBK_free(ma);
    *mem = N_NULL;
}

static void alloc_chunk(NMemAlloc* mem)
{
    NMemAllocChunk* chunk = (NMemAllocChunk*)NBK_malloc0(sizeof(NMemAllocChunk));
    int size = mem->cellSize * mem->cellsInChunk;
    chunk->mem = (uint8*)NBK_malloc(size);
    NBK_memset(chunk->mem, 0xcc, size);
    chunk->used = 0;
    
    if (mem->lst == N_NULL)
        mem->lst = sll_create();
    
    sll_append(mem->lst, chunk);
}

static nbool is_cell_free(uint8* cell, int size)
{
    nbool fre = N_TRUE;
    int i;
    
    for (i = 0; i < size; i++) {
        if (cell[i] != 0xcc) {
            fre = N_FALSE;
            break;
        }
    }
    
    return fre;
}

static nbool find_free_cell(NMemAlloc* mem, void** ptr)
{
    NMemAllocChunk* chunk = (NMemAllocChunk*)sll_first(mem->lst);
    
    while (chunk) {
        if (chunk->used < mem->cellsInChunk) {
            int i, offset = 0;
            
            for (i = 0; i < mem->cellsInChunk; i++) {
                if (is_cell_free(&chunk->mem[offset], mem->cellSize)) {
                    *ptr = &chunk->mem[offset];
                    chunk->used++;
                    return N_TRUE;
                }
                offset += mem->cellSize;
            }
        }
        
        chunk = (NMemAllocChunk*)sll_next(mem->lst);
    }
    
    return N_FALSE;
}

void* memAlloc_alloc(NMemAlloc* mem, int size)
{
    void* ptr = N_NULL;
    
    if (size != mem->cellSize)
        N_KILL_ME();
    
    if (mem->lst == N_NULL)
        alloc_chunk(mem);
    
    if (find_free_cell(mem, &ptr)) {
        // do nothing
    }
    else {
        NMemAllocChunk* chunk;
        
        alloc_chunk(mem);
        
        chunk = (NMemAllocChunk*)sll_last(mem->lst);
        ptr = &chunk->mem[0];
        chunk->used++;
    }
    
    if (ptr)
        NBK_memset(ptr, 0, size);
    
    return ptr;
}

void memAlloc_free(NMemAlloc* mem, void* ptr)
{
    NMemAllocChunk* chunk = (NMemAllocChunk*)sll_first(mem->lst);
    
    while (chunk) {
        void* s = chunk->mem;
        void* e = (uint8*)s + mem->cellSize * mem->cellsInChunk;
        
        if (ptr >= s && ptr < e) {
            int offset = (uint8*)ptr - (uint8*)s;
            NBK_memset(&chunk->mem[offset], 0xcc, mem->cellSize);
            chunk->used--;
            if (chunk->used == 0) {
                chunk = (NMemAllocChunk*)sll_removeCurr(mem->lst);
                if (chunk) {
                    NBK_free(chunk->mem);
                    NBK_free(chunk);
                }
            }
            return;
        }
        
        chunk = (NMemAllocChunk*)sll_next(mem->lst);
    }
    
    N_KILL_ME();
}
