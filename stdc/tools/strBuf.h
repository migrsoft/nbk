/*
 * strBuf.h
 *
 *  Created on: 2011-8-2
 *      Author: migr
 */

#ifndef STRBUF_H_
#define STRBUF_H_

#include "../inc/config.h"
#include "slist.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NStrBufSeg {
    char*   data;
    int     len;
} NStrBufSeg;

typedef struct _NStrBuf {
    NSList* lst;
} NStrBuf;

NStrBuf* strBuf_create(void);
void strBuf_delete(NStrBuf** strBuf);
void strBuf_reset(NStrBuf* strBuf);

// 向字符串缓冲区添加新串
void strBuf_appendStr(NStrBuf* strBuf, const char* str, int length);
// 从字符串缓冲区获取串
// begin 为 N_TRUE 时，从第一个串开始；为 N_FALSE 时，从上次获取的下一个串开始
nbool strBuf_getStr(NStrBuf* strBuf, char** str, int* len, nbool begin);
// 将所有字符串合并成一个
void strBuf_joinAllStr(NStrBuf* strBuf);

#ifdef __cplusplus
}
#endif

#endif /* STRBUF_H_ */
