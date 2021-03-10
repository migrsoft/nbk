/*
 * renderTd.h
 *
 *  Created on: 2011-9-9
 *      Author: wuyulun
 */

#ifndef RENDERTD_H_
#define RENDERTD_H_

#include "../inc/config.h"
#include "../css/cssSelector.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NRenderTd {
    
    NRenderNode d;
    
    uint8   text_align : 2;

    coord   aa_width;
    
} NRenderTd;

NRenderTd* renderTd_create(void* node);
void renderTd_delete(NRenderTd** rt);

void renderTd_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderTd_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect);

int16 renderTd_getDataSize(NRenderNode* rn, NStyle* style);

#ifdef __cplusplus
}
#endif

#endif /* RENDERTD_H_ */
