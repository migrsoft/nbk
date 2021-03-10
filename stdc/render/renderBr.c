/*
 * renderBr.c
 *
 *  Created on: 2011-1-1
 *      Author: wuyulun
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderBr_memUsed(const NRenderBr* rb)
{
    int size = 0;
    if (rb) {
        size = sizeof(NRenderBr);
    }
    return size;
}
#endif

NRenderBr* renderBr_create(void* node)
{
    NRenderBr* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderBr*)render_alloc();
#else
	r = (NRenderBr*)NBK_malloc0(sizeof(NRenderBr));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_BR;
    r->d.node = node;

    r->d.Layout = renderBr_layout;
    r->d.Paint = renderBr_paint;
    
    return r;
}

void renderBr_delete(NRenderBr** rb)
{
    NRenderBr* r = *rb;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *rb = N_NULL;
}

void renderBr_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    layoutStat_return(stat, style->font_size);

    if (rn->needLayout) {
        rn->needLayout = 0;
        rn->parent->needLayout = 1;
    }
}

void renderBr_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
}
