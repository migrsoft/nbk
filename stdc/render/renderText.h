/*
 * renderText.h
 *
 *  Created on: 2010-12-28
 *      Author: wuyulun
 */

#ifndef RENDERTEXT_H_
#define RENDERTEXT_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NRenderText {
    
    NRenderNode d;

    NFontId     fontId;
    
    nbool    underline : 1;
    
    int16   line_spacing;
    int16   indent;
    
    NRect   clip;
    
} NRenderText;

#ifdef NBK_MEM_TEST
int renderText_memUsed(const NRenderText* rt);
#endif

NRenderText* renderText_create(void* node);
void renderText_delete(NRenderText** rt);

void renderText_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderText_paint(NRenderNode* rn, NStyle*, NRenderRect* rect);

nbool renderText_hasPt(NRenderText* rt, NPoint pt);

int16 renderText_getDataSize(NRenderNode* rn, NStyle* style);

#ifdef __cplusplus
}
#endif

#endif /* RENDERTEXT_H_ */
