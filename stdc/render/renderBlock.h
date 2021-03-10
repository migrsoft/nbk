/*
 * renderBlock.h
 *
 *  Created on: 2010-12-29
 *      Author: wuyulun
 */

#ifndef RENDERBLOCK_H_
#define RENDERBLOCK_H_

#include "../inc/config.h"
#include "../css/cssSelector.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NRenderBlock {
    
    NRenderNode d;
    
    uint8   text_align : 2;
    uint8   overflow : 3;

    nbool    hasWidth : 1;
    nbool    hasHeight : 1;
    
    nbool    ne_display : 1;

    coord   aa_width;
    coord   lh;

    coord   overflow_h;

} NRenderBlock;

#ifdef NBK_MEM_TEST
int renderBlock_memUsed(const NRenderBlock* rb);
#endif

void block_align(NRenderNode* rn, nid ha);

NRenderBlock* renderBlock_create(void* node);
void renderBlock_delete(NRenderBlock** block);

void renderBlock_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderBlock_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* RENDERBLOCK_H_ */
