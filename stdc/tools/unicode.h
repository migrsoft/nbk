/*
 * unicode.h
 *
 *  Created on: 2011-3-27
 *      Author: migr
 */

#ifndef UNICODE_H_
#define UNICODE_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
int uni_utf16_to_utf8(wchr hz, char* cc);
int uni_utf16_to_utf8_len(wchr* hzs, int hzlen);
char* uni_utf16_to_utf8_str(wchr* hzs, int hzlen, int* mbLen);

wchr uni_utf8_to_utf16(uint8* mb, int8* offset);
int uni_utf8_to_utf16_len(uint8* mb, int len);
wchr* uni_utf8_to_utf16_str(uint8* mb, int len, int* wcLen);

#ifdef __cplusplus
}
#endif

#endif /* UNICODE_H_ */
