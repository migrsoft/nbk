#ifndef __NBK_CONV_H__
#define __NBK_CONV_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

int conv_gbk_to_unicode(char* mbs, int mbsLen, wchr* wcs, int wcsLen);

#ifdef __cplusplus
}
#endif

#endif // __NBK_CONV_H__
