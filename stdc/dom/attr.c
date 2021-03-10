/*
 * attr.c
 *
 *  Created on: 2011-1-2
 *      Author: wuyulun
 */

#include "attr.h"
#include "xml_helper.h"
#include "../tools/str.h"
#include "xml_atts.h"
#include "../css/css_helper.h"
#include "../css/color.h"
#include "../css/css_val.h"
#include "../css/cssSelector.h"

#ifdef NBK_MEM_TEST
int attr_memUsed(const NAttr* attr, int* styLen)
{
    int size = 0;
    
    if (styLen)
        *styLen = 0;
    
    if (attr) {
        int i, s;
        
        for (i=0; attr[i].id; i++) {
            size += sizeof(NAttr);
            if (attr[i].id == ATTID_STYLE) {
                s = cssStyle_memUsed(attr[i].d.cs, N_TRUE);
                size += s;
                if (styLen)
                    *styLen = s;
            }
            else {
                switch (attr[i].id) {
                case ATTID_ACTION:
                case ATTID_ONPICK:
                case ATTID_ONTIMER:
                case ATTID_URL:
                case ATTID_HREF:
                case ATTID_SRC:
                    s = nbk_strlen(attr[i].d.s) + 1;
                    size += s;
                    break;
                }
            }
        }
    }
    return size;
}
#endif

static int attr_num(const char** attrs)
{
    int i = 0;
    while (attrs[i])
        i += 2;
    return i / 2;
}

static nbool attr_is_alloc_str(nid attId)
{
    switch (attId) {
    case ATTID_ACTION:
    case ATTID_ONPICK:
    case ATTID_ONTIMER:
    case ATTID_URL:
    case ATTID_HREF:
    case ATTID_SRC:
    case ATTID_ONENTERFORWARD:
    case ATTID_ONENTERBACKWARD:
        return N_TRUE;
    }
    return N_FALSE;
}

static void attr_parse_tc_rect(const char* attr, NRect* rect, nbool hw)
{
    // rect in form "0,0,0,0"
    
    char *p = (char*)attr;
    char *q = p;
    
    while (*p != ',') p++;
    *p = 0;
    rect->l = NBK_atoi(q);
    *p = ',';
    
    q = ++p;
    while (*p != ',') p++;
    *p = 0;
    rect->t = NBK_atoi(q);
    *p = ',';
    
    q = ++p;
    while (*p != ',') p++;
    *p = 0;
    rect->r = NBK_atoi(q);
    *p = ',';
    
    q = ++p;
    rect->b = NBK_atoi(q);
    
    if (hw) {
        rect->r = rect->l + rect->r;
        rect->b = rect->t + rect->b;
    }
}

static void attr_parse_tc_size(const char* attr, NSize* size)
{
    // size in form "0,0"
    
    char *p = (char*)attr;
    char *q = p;
    
    while (*p != ',') p++;
    *p = 0;
    size->w = NBK_atoi(q);
    *p = ',';
    
    q = ++p;
    size->h = NBK_atoi(q);
}

// 解析属性表
static void attr_parse(NAttr* save, const char** attrs)
{
    int i, j, len;
    nid id;
    NStrPool* pool = save[0].d.spool;
    
    save[0].d.spool = N_NULL;
    for (i=j=0; attrs[i]; i+=2) {
        
        id = get_att_id(attrs[i]);
        
        switch (id) {
        case ATTID_MULTIPLE:
        case ATTID_DISABLED:
        case ATTID_CHECKED:
        case ATTID_SELECTED:
        case ATTID_TC_MAINTOPIC:
        case ATTID_TC_ONFOCUS:
            save[j].id = id;
            save[j].d.b = N_TRUE;
            j++;
            break;
        }
        
        if (attrs[i+1] == N_NULL) {
            if (id == ATTID_VALUE) {
                save[j].id = id;
                save[j].d.ps = N_NULL;
                j++;
            }
            continue;
        }
        
        // must has value
        
        switch (id) {
        case ATTID_SIZE:
        case ATTID_MAXLENGTH:
        case ATTID_COLS:
        case ATTID_ROWS:
        case ATTID_BORDER:
        case ATTID_CELLSPACING:
        case ATTID_CELLPADDING:
        case ATTID_TC_CUTEOFFSETX:
        case ATTID_TC_CUTEORIGINW:
            save[j].id = id;
            save[j].d.v = NBK_atoi(attrs[i+1]);
            j++;
            break;
            
        case ATTID_ACTION:
        case ATTID_ONPICK:
        case ATTID_ONTIMER:
        case ATTID_URL:
        case ATTID_HREF:
        case ATTID_SRC:
        case ATTID_ONENTERFORWARD:
        case ATTID_ONENTERBACKWARD:
            save[j].id = id;
            len = nbk_strlen(attrs[i+1]);
            save[j].d.s = (char*)NBK_malloc(len + 1);
            nbk_strcpy(save[j].d.s, attrs[i+1]);
            j++;
            break;
            
        case ATTID_COLOR:
            save[j].id = id;
            save[j].d.c = colorBlack;
            css_parseColor(attrs[i+1], -1, &save[j].d.c);
            j++;
            break;
            
        case ATTID_BGCOLOR:
            save[j].id = id;
            save[j].d.c = colorWhite;
            css_parseColor(attrs[i+1], -1, &save[j].d.c);
            j++;
            break;
            
        case ATTID_STYLE:
            save[j].id = id;
            save[j].d.cs = sheet_parseInlineStyle(attrs[i+1]);
            j++;
            break;
            
        case ATTID_ALIGN:
            save[j].id = id;
            save[j].d.t = css_parseAlign(attrs[i+1], -1);
            j++;
            break;
            
        case ATTID_CLASS:
        case ATTID_ID:
        case ATTID_NAME:
        case ATTID_VALUE:
        case ATTID_METHOD:
        case ATTID_TYPE:
        case ATTID_TITLE:
        case ATTID_CHARSET:
        case ATTID_ENCTYPE:
        case ATTID_CONTENT:
        case ATTID_HTTP_EQUIV:
        case ATTID_ALT:
        case ATTID_REL:
        case ATTID_TC_NAME:
        case ATTID_TC_FILENAME:
        case ATTID_NE_DISPLAY:
            save[j].id = id;
            save[j].d.ps = strPool_save(pool, attrs[i+1], -1);
            j++;
            break;

        case ATTID_TC_RECT:
            save[j].id = id;
            attr_parse_tc_rect(attrs[i+1], &save[j].d.r, N_TRUE);
            j++;
            break;
            
        case ATTID_TC_PCLIP:
            save[j].id = id;
            attr_parse_tc_rect(attrs[i+1], &save[j].d.r, N_FALSE);
            j++;
            break;
            
        case ATTID_TC_ORIGINRECT:
            save[j].id = id;
            attr_parse_tc_size(attrs[i+1], &save[j].d.sz);
            j++;
            break;
            
        case ATTID_TC_IA:
        case ATTID_TC_NIDX:
        case ATTID_TC_ISADS:
        case ATTID_TC_STRETCH:
            save[j].id = id;
            save[j].d.i32 = NBK_atoi(attrs[i+1]);
            j++;
            break;
            
        case ATTID_WIDTH:
        case ATTID_HEIGHT:
        case ATTID_TC_RATE:
            save[j].id = id;
            css_parseNumber(attrs[i+1], -1, &save[j].d.cv);
            j++;
            break;
        }
    }
}

NAttr* attr_create(NStrPool* pool, const char** attrs)
{
    NAttr* na;
    int num = attr_num(attrs);
    if (num == 0)
        return N_NULL;
    
    na = (NAttr*)NBK_malloc0(sizeof(NAttr) * (num + 1));
    N_ASSERT(na);
    na[0].d.spool = pool;
    attr_parse(na, attrs);
    return na;
}

void attr_delete(NAttr** nattrs)
{
    NAttr* na = *nattrs;
    int i;
    
    for (i=0; na[i].id; i++) {
        if (attr_is_alloc_str(na[i].id)) {
            NBK_free(na[i].d.s);
        }
        else if (na[i].id == ATTID_STYLE) {
            if (na[i].d.cs->bgImage)
                NBK_free(na[i].d.cs->bgImage);
            NBK_free(na[i].d.cs);
        }
    }
    NBK_free(na);
    *nattrs = N_NULL;
}

char* attr_getValueStr(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return N_NULL;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId) {
            if (attr_is_alloc_str(attId))
                return nattrs[i].d.s;
            else
                return nattrs[i].d.ps;
        }
    }
    
    return N_NULL;
}

void attr_setValueStr(NAttr* nattrs, nid attId, NStrPool* spool, char* str)
{
    int i;
    
    if (nattrs == N_NULL)
        return;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId) {
            if (attr_is_alloc_str(attId)) {
                int len = nbk_strlen(str);
                if (nattrs[i].d.s)
                    NBK_free(nattrs[i].d.s);
                nattrs[i].d.s = (char*)NBK_malloc(len + 1);
                nbk_strcpy(nattrs[i].d.s, str);
            }
            else {
                nattrs[i].d.ps = strPool_save(spool, str, -1);
            }
            break;
        }
    }
}

coord attr_getCoord(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return -1;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId)
            return nattrs[i].d.v;
    }
    
    return -1;
}

NRect* attr_getRect(NAttr* nattrs, nid attId)
{
    if (nattrs) {
        int i;
        for (i=0; nattrs[i].id; i++) {
            if (nattrs[i].id == attId)
                return &nattrs[i].d.r;
        }
    }
    
    return N_NULL;
}

NSize* attr_getSize(NAttr* nattrs, nid attId)
{
    if (nattrs) {
        int i;
        for (i=0; nattrs[i].id; i++) {
            if (nattrs[i].id == attId)
                return &nattrs[i].d.sz;
        }
    }
    
    return N_NULL;
}

nbool attr_getValueBool(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return N_FALSE;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId)
            return nattrs[i].d.b;
    }
    return N_FALSE;
}

void attr_setValueBool(NAttr* nattrs, nid attId, nbool value)
{
    int i;
    
    if (nattrs == N_NULL)
        return;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId) {
            nattrs[i].d.b = value;
            break;
        }
    }
}

int32 attr_getValueInt32(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return -1;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId)
            return nattrs[i].d.i32;
    }
    
    return -1;
}

NCssStyle* attr_getStyle(NAttr* nattrs)
{
    int i;
    
    if (nattrs == N_NULL)
        return N_NULL;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == ATTID_STYLE)
            return nattrs[i].d.cs;
    }
    
    return N_NULL;
}

NColor* attr_getColor(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return N_NULL;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId)
            return &nattrs[i].d.c;
    }
    
    return N_NULL;
}

nid attr_getType(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return N_INVALID_ID;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId)
            return nattrs[i].d.t;
    }
    
    return N_INVALID_ID;
}

NCssValue* attr_getCssValue(NAttr* nattrs, nid attId)
{
    int i;
    
    if (nattrs == N_NULL)
        return N_NULL;
    
    for (i=0; nattrs[i].id; i++) {
        if (nattrs[i].id == attId)
            return &nattrs[i].d.cv;
    }
    
    return N_NULL;
}
