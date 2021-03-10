/*
 * css_helper.h
 *
 *  Created on: 2011-1-4
 *      Author: wuyulun
 */

#ifndef CSS_HELPER_H_
#define CSS_HELPER_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../css/cssSelector.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NCssStr {
    char*   s;
    int16   l;
} NCssStr;

typedef struct _NCssDecl {
    NCssStr p;
    NCssStr v;
} NCssDecl;

nbool css_parseColor(const char* colorStr, int len, NColor* color);
uint8 css_parseAlign(const char* str, int len);
void css_parseNumber(const char* str, int len, NCssValue* val);

nid css_getPropertyId(const char* prop, int len);
int css_parseDecl(const char* style, int len, NCssDecl decl[], int max);
int css_parseValues(const char* str, int len, NCssValue vals[], int max);

void css_addStyle(NCssStyle* s1, const NCssStyle* s2);

coord css_calcBgPos(coord w, coord bgw, NCssValue bgpos, nbool repeat);
coord css_calcValue(NCssValue val, int32 base, void* style, int32 defVal);

coord calcHoriAlignDelta(coord maxw, coord w, nid ha);
coord calcVertAlignDelta(coord maxb, coord b, nid va);

void css_boxAddSide(NCssBox* dst, const NCssBox* src);

void cssVal_setValue(NCssValue* cv, nid id);
void cssVal_setInt(NCssValue* cv, int i);
void cssVal_setPercent(NCssValue* cv, int16 percent);

nbool cssVal_same(const NCssValue* cv1, const NCssValue* cv2);

#ifdef __cplusplus
}
#endif

#endif /* CSS_HELPER_H_ */
