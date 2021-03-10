#ifndef __UF_GBK_H__
#define __UF_GBK_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

wchr unicode_chr_from_gbk(char* han);

int unicode_str_from_gbk(char* mbs, int mbsLen, wchr* wcs, int wcsLen);

#ifdef __cplusplus
}
#endif

#endif // __UF_GBK_H__
