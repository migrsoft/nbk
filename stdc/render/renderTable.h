/*
 * renderTable.h
 *
 *  Created on: 2011-9-9
 *      Author: wuyulun
 */

#ifndef RENDERTABLE_H_
#define RENDERTABLE_H_

#include "../inc/config.h"
#include "../css/cssSelector.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NRenderTable {
    
    NRenderNode d;
    
    nbool    hasWidth : 1;
    
    int16   cellpadding;
    
    int16   cols_num;
    int16*  cols_width;

    coord   aa_width;
    
} NRenderTable;

NRenderTable* renderTable_create(void* node);
void renderTable_delete(NRenderTable** rt);

void renderTable_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderTable_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect);

coord renderTable_getColWidth(NRenderTable* table, int16 col);

#ifdef __cplusplus
}
#endif

#endif /* RENDERTABLE_H_ */
