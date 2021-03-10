/*
 * renderBlank.h
 *
 *  Created on: 2011-5-2
 *      Author: migr
 */

#ifndef RENDERBLANK_H_
#define RENDERBLANK_H_

#include "../inc/config.h"
#include "../css/cssSelector.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NRenderBlank {
    
    NRenderNode d;
    
    uint8   overflow : 3;
    
} NRenderBlank;

#ifdef NBK_MEM_TEST
int renderBlank_memUsed(const NRenderBlank* rb);
#endif

NRenderBlank* renderBlank_create(void* node);
void renderBlank_delete(NRenderBlank** render);

void renderBlank_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderBlank_paint(NRenderNode* rn, NStyle*, NRenderRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* RENDERBLANK_H_ */
