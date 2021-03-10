/*
 * attr.h
 *
 *  Created on: 2011-1-2
 *      Author: wuyulun
 */

#ifndef ATTR_H_
#define ATTR_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../css/cssSelector.h"
#include "../tools/strPool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NAttr {
    
    nid     id;
    
    union {
        nbool        b;
        coord       v;
        char*       s;
        NColor      c;
        NSize       sz;
        NRect       r;
        nid         t;
        int32       i32;
        NCssStyle*  cs;
        char*       ps; // string in pool
        NCssValue   cv;
        NStrPool*   spool;
    } d;
    
} NAttr;

#ifdef NBK_MEM_TEST
int attr_memUsed(const NAttr* attr, int* styLen);
#endif

NAttr* attr_create(NStrPool* pool, const char** attrs);
void attr_delete(NAttr** nattrs);

char* attr_getValueStr(NAttr* nattrs, nid attId);
void attr_setValueStr(NAttr* nattrs, nid attId, NStrPool* spool, char* str);

coord attr_getCoord(NAttr* nattrs, nid attId);
NRect* attr_getRect(NAttr* nattrs, nid attId);
NSize* attr_getSize(NAttr* nattrs, nid attId);

nbool attr_getValueBool(NAttr* nattrs, nid attId);
void attr_setValueBool(NAttr* nattrs, nid attId, nbool value);

int32 attr_getValueInt32(NAttr* nattrs, nid attId);

NCssStyle* attr_getStyle(NAttr* nattrs);
NColor* attr_getColor(NAttr* nattrs, nid attId);
nid attr_getType(NAttr* nattrs, nid attId);
NCssValue* attr_getCssValue(NAttr* nattrs, nid attId);

#ifdef __cplusplus
}
#endif

#endif /* ATTR_H_ */
