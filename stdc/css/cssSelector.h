/*
 * cssSelector.h
 *
 *  Created on: 2011-1-9
 *      Author: wuyulun
 */

#ifndef __CSSSELECTOR_H__
#define __CSSSELECTOR_H__

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../dom/xml_tags.h"
#include "../tools/strPool.h"
#include "../tools/strDict.h"
#include "../tools/strBuf.h"
#include "../tools/slist.h"
#include "../tools/ptrArray.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef NRect NCssBox;

enum NECssValueType {
    NECVT_NONE,
    NECVT_COLOR,
    NECVT_VALUE,
    NECVT_INT,
    NECVT_EM,
    NECVT_PERCENT,
    NECVT_URL,
    NECVT_FLOAT
};

typedef struct _NCssValue {
    
    nid         type;
    
    union {
        NColor  color;
        nid     value;
        int32   i32;
        int16   perc;
        char*   url;
        float	flo;
    } d;
    
} NCssValue;

typedef struct _NCssStyle {
    
    uint8   flo : 2;
    uint8   clr : 2;
    
    uint8   text_align : 2;
    uint8   vert_align : 2;
    uint8   font_weight : 2;
    
    uint8   bg_repeat : 2;
    uint8   bg_x_t : 2;
    uint8   bg_y_t : 2;
    
    uint8   display;
    
    uint8   overflow;
    uint8   text_decor;
    
    // flags
    uint16  hasColor : 1;
    uint16  hasBgcolor : 1;
    uint16  hasBrcolorL : 1;
    uint16  hasBrcolorT : 1;
    uint16  hasBrcolorR : 1;
    uint16  hasBrcolorB : 1;
    
    uint16  hasMargin : 1;
    uint16  hasBorder : 1;
    uint16  hasPadding : 1;
    
    uint16  hasTextAlign : 1;
    uint16  hasOverflow : 1;
    uint16  hasTextDecor : 1;
    uint16  hasVertAlign : 1;
    uint16  hasOpacity : 1;
    
    NColor  color;
    NColor  background_color;
    NColor  border_color_l;
    NColor  border_color_t;
    NColor  border_color_r;
    NColor  border_color_b;
    
    NCssBox margin;
    NCssBox border;
    NCssBox padding;
    
    char*   bgImage;
    coord   bg_x;
    coord   bg_y;
    
    int32   z_index;

    NCssValue   font_size;
    
    NCssValue   width;
    NCssValue   height;
    NCssValue   max_width;
    
    NCssValue   text_indent;
    NCssValue   line_height;
    
    float		opacity;
    
} NCssStyle;

enum NESelType {
    SELT_TAG,
    SELT_CLASS,
    SELT_ID,
    SELT_TAG_CLASS,
    SELT_TAG_ID,
    SELT_ALL,
    SELT_UNUSED
};

typedef struct _NSelCell {
    
    nid     tid;
    uint8   type;
    nbool    hasChild;
    char*   txt;
    
    union {
        struct _NSelCell*   child;
        NCssStyle*          style;
    } d;
    
} NSelCell;

typedef struct _NSheet {
    
    NCssStyle*  ts[TAGID_LASTTAG]; // univerial selector
    NStrDict*   cs; // class selector
    NStrDict*   is; // id selector
    NSList*     ds; // descendant selector
    
    NStrPool*   strPool;
    NPtrArray*  bgimgs;
    
    NStrBuf*    data;
    
    void*       page; // not owned
    
} NSheet;

#ifdef NBK_MEM_TEST
int cssStyle_memUsed(const NCssStyle* style, nbool ownBg);
int sheet_memUsed(const NSheet* sheet);
#endif

NSheet* sheet_create(void* page);
void sheet_delete(NSheet** sheet);
void sheet_reset(NSheet* sheet);

void sheet_addData(NSheet* sheet, const char* style, int length);
void sheet_parse(NSheet* sheet);

void sheet_parseStyle(NSheet* sheet, const char* style, int length);
NCssStyle* sheet_parseInlineStyle(const char* style);

void sheet_getStyle(NSheet* sheet, NSelCell tagsc, NSelCell* des[], NCssStyle* style);

void sheet_dump(NSheet* sheet);

#ifdef __cplusplus
}
#endif

#endif /* __CSSSELECTOR_H__ */
