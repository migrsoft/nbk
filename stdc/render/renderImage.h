/*
 * renderImage.h
 *
 *  Created on: 2010-12-28
 *      Author: wuyulun
 */

#ifndef RENDERIMAGE_H_
#define RENDERIMAGE_H_

#include "../inc/config.h"
#include "renderNode.h"

#define IMAGE_STOCK_TYPE_ID  32700

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NRenderImage {

    NRenderNode d;
    
    nbool        fail;
    int16       iid;
    
    NRect       clip;
    
} NRenderImage;

#ifdef NBK_MEM_TEST
int renderImage_memUsed(const NRenderImage* ri);
#endif

NRenderImage* renderImage_create(void* node);
void renderImage_delete(NRenderImage** ri);

void renderImage_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderImage_paint(NRenderNode* rn, NStyle*, NRenderRect* rect);

int16 renderImage_getDataSize(NRenderNode* rn, NStyle* style);

#ifdef __cplusplus
}
#endif

#endif /* RENDERIMAGE_H_ */
