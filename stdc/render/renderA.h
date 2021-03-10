/*
 * renderA.h
 *
 *  Created on: 2011-8-22
 *      Author: wuyulun
 */

#ifndef RENDERA_H_
#define RENDERA_H_

#include "../inc/config.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NRenderA {
    
    NRenderNode     d;

    uint8   overflow : 3;

} NRenderA;

NRenderA* renderA_create(void* node);
void renderA_delete(NRenderA** ra);

void renderA_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderA_paint(NRenderNode* rn, NStyle* style, NRect* rect);

#ifdef __cplusplus
}
#endif

#endif /* RENDERA_H_ */
