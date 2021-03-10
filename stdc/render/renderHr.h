/*
 * renderHr.h
 *
 *  Created on: 2011-4-4
 *      Author: migr
 */

#ifndef RENDERHR_H_
#define RENDERHR_H_

#include "../inc/config.h"
#include "../css/cssSelector.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NRenderHr {
    
    NRenderNode d;

    coord   size;
    
} NRenderHr;

#ifdef NBK_MEM_TEST
int renderHr_memUsed(const NRenderHr* rh);
#endif

NRenderHr* renderHr_create(void* node);
void renderHr_delete(NRenderHr** rh);

void renderHr_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderHr_paint(NRenderNode* rn, NStyle*, NRenderRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* RENDERHR_H_ */
