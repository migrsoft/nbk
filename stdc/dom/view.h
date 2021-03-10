/*
 * view.h
 *
 *  Created on: 2010-12-29
 *      Author: wuyulun
 */

#ifndef VIEW_H_
#define VIEW_H_

#include "../inc/config.h"
#include "../inc/nbk_limit.h"
#include "../render/layoutStat.h"
#include "../render/renderNode.h"
#include "../render/imagePlayer.h"
#include "node.h"

#ifdef __cplusplus
extern "C" {
#endif

// render helper

typedef enum _NERenderKind {
    NERK_NONE,
    NERK_TEXT,
    NERK_HYPERLINK,
    NERK_IMAGE,
    NERK_IMAGE_HYPERLINK,
    NERK_EDITOR,
    NERK_CONTROL
} NERenderKind;

// CSS z-index info

typedef struct _NZidxCell {
    NRenderNode*    rn;
    NRect           rect;
    nbool            normal;
} NZidxCell;

typedef struct _NZidxPanel {
    int32       z;
    int16       use;
    int16       max;
    NZidxCell*  cells;
} NZidxPanel;

typedef struct _NZidxMatrix {
    nbool        ready;
    int16       use;
    int16       max;
    NZidxPanel* panels;
} NZidxMatrix;

// View / Render Tree

typedef struct _NView {
    
    void*           page; // not owned
    
    NRenderNode*    root;
    
    NLayoutStat*    stat;
    NRenderNode*    lastChild;
    
    coord   scrWidth; // init layout width
    coord   optWidth; // mobile optimized width
    NRect   overflow; // contents overflow
    
    NNode*  focus;
    
    NImagePlayer*   imgPlayer;
    
    nbool    ownPlayer : 1;
    nbool    drawPic : 1;
    nbool    drawBgimg : 1;
    
    uint32	paintMaxTime;
    uint32	paintTime;
    
    NZidxMatrix*    zidx;
    
    NRect   frameRect; // ff only

    int     _r_no;
        
} NView;

#ifdef NBK_MEM_TEST
int view_memUsed(const NView* view);
#endif

NView* view_create(void* page);
void view_delete(NView** view);

void view_begin(NView* view);
void view_clear(NView* view);

void view_processNode(NView* view, NNode* node);

void view_layout(NView* view, NRenderNode* root, nbool force);
//nbool view_layoutTextFF(NView* view);

void view_paint(NView* view, NRect* rect);
void view_paintFocus(NView* view, NRect* rect);
void view_paintControl(NView* view, NRect* rect);
void view_paintMainBodyBorder(NView* view, NNode* node, NRect* rect,nbool boder);
void view_paintPartially(NView* view, NRenderNode* root, NRect* rect);
void view_paintWithZindex(NView* view, NRect* rect);
void view_paintFocusWithZindex(NView* view, NRect* rect);

void view_picChanged(NView* view, int16 imgId);
void view_picsChanged(NView* view, NSList* lst);
void view_picUpdate(NView* view, int16 imgId);

NRect view_getRectWithOverflow(NView* view);
coord view_width(NView* view);
coord view_height(NView* view);
float view_getMinZoom(NView* view);

void view_setFocusNode(NView* view, NNode* node);
void* view_getFocusNode(NView* view);
NNode* view_findFocusWithZindex(NView* view, coord x, coord y);

NRect view_getNodeRect(NView* view, NNode* node);

void view_enablePic(NView* view, nbool enable);
void view_stop(NView* view);
void view_pause(NView* view);
void view_resume(NView* view);

void view_dump_render_tree_2(NView* view, NRenderNode* root);

void rtree_delete_node(NView* view, NRenderNode* node);
nbool rtree_replace_node(NView* view, NRenderNode* old, NRenderNode* novice);
nbool rtree_insert_node(NRenderNode* node, NRenderNode* novice, nbool before);
nbool rtree_insert_node_as_child(NRenderNode* parent, NRenderNode* novice);

void view_nodeDisplayChanged(NView* view, NRenderNode* rn, nid display);
void view_nodeDisplayChangedFF(NView* view, NRenderNode* rn, nid display);
void view_checkFocusVisible(NView* view);

void view_buildZindex(NView* view);
nbool view_isZindex(NView* view);
void view_clearZindex(NView* view);

// helper

void view_normalizePoint(NView* view, NPoint* pos, NRect va, float zoom);

// 查找指定位置的Render结点
NRenderNode* view_findRenderByPos(NView* view, const NPoint* pos);
// 返回Render结点的功能类型
NERenderKind view_getRenderKind(NRenderNode* rn);
// 返回图片原始URL
const char* view_getRenderSrc(NRenderNode* rn);

#ifdef __cplusplus
}
#endif

#endif /* VIEW_H_ */
