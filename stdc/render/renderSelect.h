/*
 * renderSelect.h
 *
 *  Created on: 2011-4-6
 *      Author: wuyulun
 */

#ifndef RENDERSELECT_H_
#define RENDERSELECT_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../tools/slist.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NRenderSelect {
    
    NRenderNode d;
    
    NFontId     fontId;
    
    nbool        multiple : 1;
    nbool        expand : 1;
    nbool        empty : 1;
    
    nbool        modal : 1; // specific for paint
    nbool        move : 1;
    
    NSList*     lst;
    
    NRect       vr; // rect of popup
    coord       w; // width of popup
    coord       h; // height of popup
    coord       lastY;
    
    int16       visi;
    int16       items;
    int16       top;
    int16       cur;
    
} NRenderSelect;

#ifdef NBK_MEM_TEST
int renderSelect_memUsed(const NRenderSelect* rs);
#endif

NRenderSelect* renderSelect_create(void* node);
void renderSelect_delete(NRenderSelect** select);

void renderSelect_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderSelect_paint(NRenderNode* rn, NStyle*, NRect* rect);

nbool renderSelect_popup(NRenderSelect* select);
void renderSelect_dismiss(NRenderSelect* select);
nbool renderSelect_processKey(NRenderNode* rn, NEvent* event);

int16 renderSelect_getDataSize(NRenderNode* rn, NStyle* style);

#ifdef __cplusplus
}
#endif

#endif /* RENDERSELECT_H_ */
