#ifndef __RENDERINLINEBLOCK_H__
#define __RENDERINLINEBLOCK_H__

#include "../inc/config.h"
#include "renderNode.h"
#include "renderBlock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef NRenderBlock NRenderInlineBlock;

NRenderInlineBlock* renderInlineBlock_create(void* node);
void renderInlineBlock_delete(NRenderInlineBlock** iblock);

void renderInlineBlock_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderInlineBlock_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect);

#ifdef __cplusplus
}
#endif

#endif
