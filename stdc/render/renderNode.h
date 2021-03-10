/*
 * renderNode.h
 *
 *  Created on: 2010-12-28
 *      Author: wuyulun
 */

#ifndef RENDERNODE_H_
#define RENDERNODE_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../dom/node.h"
#include "../css/cssSelector.h"
#include "layoutStat.h"

#ifdef __cplusplus
extern "C" {
#endif

enum NERenderNodeType {
    
    RNT_NONE,
    RNT_BLANK,
    RNT_BLOCK,
	RNT_INLINE_BLOCK,
    RNT_INLINE,
    RNT_TEXT,
    RNT_TEXTPIECE,
    RNT_IMAGE,
    RNT_INPUT,
    RNT_BR,
    RNT_HR,
    RNT_SELECT,
    RNT_TEXTAREA,
    RNT_OBJECT,
    RNT_A,
    RNT_TABLE,
    RNT_TR,
    RNT_TD,
    RNT_LAST
};

typedef NRect NRenderRect;

typedef struct _NStyle {
    
    int16   font_size;
    
    uint8   display;
    uint8   flo : 2;
    uint8   clr : 2;
    
    uint8   bold : 1;
    uint8   italic : 1;
    uint8   text_align : 2;
    uint8   vert_align : 2;
    uint8   highlight : 1;
    uint8   underline : 1;

    uint8   bg_repeat : 2;
    uint8   bg_x_t : 2;
    uint8   bg_y_t : 2;
    uint8   lh_t : 2;
    
    uint8   belongA : 1; // for draw contents selected by A element
    uint8   drawPic : 1;
    uint8   drawBgimg : 1;
    
    // for NBK extend
    uint8   ne_fold : 1;
    
    uint8   overflow;
    
    uint8   hasBgcolor : 1;
    uint8   hasBrcolorL : 1;
    uint8   hasBrcolorT : 1;
    uint8   hasBrcolorR : 1;
    uint8   hasBrcolorB : 1;

    NColor  color;
    NColor  background_color;

    NColor  border_color_l;
    NColor  border_color_t;
    NColor  border_color_r;
    NColor  border_color_b;
    
    // control
    NColor  ctlTextColor;
    NColor  ctlFgColor;
    NColor  ctlBgColor;
    NColor  ctlFillColor;
    NColor  ctlFocusColor;
    NColor  ctlEditColor;
    
    NCssBox margin;
    NCssBox border;
    NCssBox padding;
    
    char*   bgImage;
    coord   bg_x;
    coord   bg_y;
    
    int32   z_index;
    
    NCssValue   width;
    NCssValue   height;
    NCssValue   max_width;
    
    NCssValue   text_indent;
    NCssValue   line_height;
    
    float	opacity;
    
    // other
    void*   view;
    NRect   dv;
    float	zoom;
    
    // for clip calc
    NRect*  clip;
    
} NStyle;

void style_init(NStyle* style);

typedef struct _NRenderNode {

    int     _no; // debug only
    
    uint8   type;
    
    uint8   init : 1;
    uint8   force : 1;
    uint8   needLayout : 1;
    uint8   zindex : 1;
    uint8   isAd : 1; // it's an AD
    
    uint8   display : 2;
    uint8   flo : 2;
    uint8   clr : 2;
    uint8   vert_align : 2;
    uint8   bg_repeat : 2;
    uint8   bg_x_t : 2;
    uint8   bg_y_t : 2;
    
    uint8   hasBgcolor : 1;
    uint8   hasBrcolorL : 1;
    uint8   hasBrcolorT : 1;
    uint8   hasBrcolorR : 1;
    uint8   hasBrcolorB : 1;

    NRect   r; // box
    NRect   og_r; // original box
    NRect   aa_r;
    
	// box 模型
    NCssBox margin;
    NCssBox border;
    NCssBox padding;

    // for color
    NColor  color;
    NColor  background_color;

    NColor  border_color_l;
    NColor  border_color_t;
    NColor  border_color_r;
    NColor  border_color_b;
    
    // for background image
    int16   bgiid;
    
    coord   bg_x;
    coord   bg_y;

    void*   node; // not owned, type is Node, may be NULL.
    
    struct _NRenderNode* parent;
    struct _NRenderNode* child;
    struct _NRenderNode* prev;
    struct _NRenderNode* next;
    
    void (*Layout)(NLayoutStat* stat, struct _NRenderNode* rn, NStyle* style, nbool force);
    void (*Paint)(struct _NRenderNode* rn, NStyle* style, NRect* rect);
        
} NRenderNode;

#ifdef NBK_MEM_TEST
int renderNode_memUsed(const NRenderNode* rn);
#endif

NRenderNode* renderNode_create(void* view, NNode* node);
void renderNode_delete(NRenderNode** rn);
void renderNode_init(NRenderNode* rn);
void renderNode_calcStyle(NRenderNode* rn, NStyle* style);
nbool renderNode_layoutable(NRenderNode* rn, coord maxw);
nbool renderNode_isChildsNeedLayout(NRenderNode* rn);

nbool renderNode_has(NNode* node, nid type);

void renderNode_adjustPos(NRenderNode* rn, coord dy);

void renderNode_absRect(NRenderNode* rn, NRect* rect);
nbool renderNode_getAbsRect(NRenderNode* rn, NRect* rect);

int renderNode_getEditLength(NRenderNode* rn);
int renderNode_getEditMaxLength(NRenderNode* rn);
nbool renderNode_getRectOfEditing(NRenderNode* rn, NRect* rect, NFontId* fontId);

wchr* renderNode_getEditText16(NRenderNode* rn, int* len);
void renderNode_setEditText16(NRenderNode* rn, wchr* text, int len);

void renderNode_getSelPos(NRenderNode* rn, int* start, int* end);
void renderNode_setSelPos(NRenderNode* rn, int start, int end);

void renderNode_drawBorder(NRenderNode* rn, NCssBox* border, NRect* rect, void* pfd);
void renderNode_drawBgImage(NRenderNode* rn, NRect* pr, NRect* vr, NStyle* style);
void renderNode_drawFocusRing(NRect pr, NColor color, void* pfd);

void renderNode_fillButton(const NRect* pr, void* pfd);

// 渲染树深度遍历工具
typedef int (*RTREE_TRAVERSE_START_CB)(NRenderNode* render, void* user, nbool* ignore);
typedef int (*RTREE_TRAVERSE_CB)(NRenderNode* render, void* user);
void rtree_traverse_depth(NRenderNode* root, RTREE_TRAVERSE_START_CB startCb, RTREE_TRAVERSE_CB closeCb, void* user);

NRenderNode* renderNode_getParent(NRenderNode* rn, uint8 type);
void renderNode_getAbsPos(NRenderNode* rn, coord* x, coord* y);

NRenderNode* rtree_get_last_sibling(NRenderNode* rn);

NRenderNode* renderNode_getFirstChild(NRenderNode* rn);
NRenderNode* renderNode_getLastChild(NRenderNode* rn);
NRenderNode* renderNode_getPrevNode(NRenderNode* rn);
NRenderNode* renderNode_getNextNode(NRenderNode* rn);

coord renderNode_getInnerWidth(NRenderNode* rn);
coord renderNode_getContainerWidth(NRenderNode* rn);

coord renderNode_getLeftMBP(NRenderNode* rn);
coord renderNode_getTopMBP(NRenderNode* rn);
coord renderNode_getRightMBP(NRenderNode* rn);
coord renderNode_getBottomMBP(NRenderNode* rn);

NRect renderNode_getBorderBox(NRenderNode* rn);
NRect renderNode_getContentBox(NRenderNode* rn);

nbool renderNode_hasBorder(NRenderNode* rn);

void renderNode_transform(NRenderNode* rn, NStyle* style);

nbool renderNode_isBlockElement(const NRenderNode* rn);

NRect renderNode_getPaintArea(NRenderNode* rn);

nbool renderNode_isRenderDisplay(const NRenderNode* rn);

#ifdef __cplusplus
}
#endif

#endif /* RENDERNODE_H_ */
