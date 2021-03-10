/*
 * renderInline.h
 *
 *  Created on: 2011-10-31
 *      Author: wuyulun
 */

#ifndef RENDERINLINE_H_
#define RENDERINLINE_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../css/cssSelector.h"
#include "renderNode.h"
#include "renderBlock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef NRenderBlock NRenderInline;

NRenderInline* renderInline_create(void* node);
void renderInline_delete(NRenderInline** ri);

void renderInline_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderInline_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* RENDERINLINE_H_ */
