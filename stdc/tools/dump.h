/*
 * probe.h
 *
 *  Created on: 2011-2-18
 *      Author: wuyulun
 */

#ifndef DUMP_H_
#define DUMP_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif
    
void dump_int(void* page, int32 i);
void dump_uint(void* page, uint32 i);
void dump_char(void* page, char* mb, int length);
void dump_wchr(void* page, wchr* wc, int length);
void dump_return(void* page);
void dump_time(void* page);
void dump_flush(void* page);
void dump_tab(void* page, nbool on);

void dump_NColor(void* page, const NColor* color);
void dump_NRect(void* page, const NRect* rect);
void dump_NRectSides(void* page, const NRect* rect);
void dump_NCssBox(void* page, const void* box);
void dump_NCssValue(void* page, const void* cssVal);

#ifdef __cplusplus
}
#endif

#endif /* DUMP_H_ */
