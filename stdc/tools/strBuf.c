/*
 * strBuf.c
 *
 *  Created on: 2011-8-2
 *      Author: migr
 */

#include "strBuf.h"
#include "str.h"

NStrBuf* strBuf_create(void)
{
    NStrBuf* sb = (NStrBuf*)NBK_malloc0(sizeof(NStrBuf));
    N_ASSERT(sb);
    return sb;
}

void strBuf_delete(NStrBuf** strBuf)
{
    NStrBuf* sb = *strBuf;
    strBuf_reset(sb);
    NBK_free(sb);
    *strBuf = N_NULL;
}

void strBuf_reset(NStrBuf* strBuf)
{
    if (strBuf->lst) {
        NStrBufSeg* seg = sll_first(strBuf->lst);
        while (seg) {
            NBK_free(seg->data);
            NBK_free(seg);
            seg = sll_next(strBuf->lst);
        }
        sll_delete(&strBuf->lst);
    }
}

void strBuf_appendStr(NStrBuf* strBuf, const char* str, int length)
{
    int len = (length == -1) ? nbk_strlen(str) : length;
    NStrBufSeg* seg = (NStrBufSeg*)NBK_malloc0(sizeof(NStrBufSeg));
    seg->len = len;
    seg->data = (char*)NBK_malloc(len);
    NBK_memcpy(seg->data, (void*)str, len);
    
    if (strBuf->lst == N_NULL)
        strBuf->lst = sll_create();
    sll_append(strBuf->lst, seg);
}

nbool strBuf_getStr(NStrBuf* strBuf, char** str, int* len, nbool begin)
{
    NStrBufSeg* seg;
    
    if (strBuf->lst == N_NULL)
        return N_FALSE;
    
    seg = (begin) ? sll_first(strBuf->lst) : sll_next(strBuf->lst);
    if (seg) {
        *str = seg->data;
        *len = seg->len;
        return N_TRUE;
    }
    else
        return N_FALSE;
}

void strBuf_joinAllStr(NStrBuf* strBuf)
{
    char* all;
    int len = 0, c = 0;
    NStrBufSeg* seg;
    
    if (strBuf->lst == N_NULL)
        return;
    
    seg = sll_first(strBuf->lst);
    while (seg) {
        len += seg->len;
        c++;
        seg = sll_next(strBuf->lst);
    }
    
    if (len == 0 || c == 1)
        return;
    
    all = (char*)NBK_malloc(len);
    len = 0;
    seg = sll_first(strBuf->lst);
    while (seg) {
        NBK_memcpy(all + len, seg->data, seg->len);
        len += seg->len;
        seg = sll_next(strBuf->lst);
    }
    strBuf_reset(strBuf);
    
    strBuf->lst = sll_create();
    seg = (NStrBufSeg*)NBK_malloc0(sizeof(NStrBufSeg));
    seg->data = all;
    seg->len = len;
    sll_append(strBuf->lst, seg);
}
