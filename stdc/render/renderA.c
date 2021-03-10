/*
 * renderA.c
 *
 *  Created on: 2011-8-22
 *      Author: wuyulun
 */

#include "renderInc.h"

NRenderA* renderA_create(void* node)
{
    NRenderA* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderA*)render_alloc();
#else
	r = (NRenderA*)NBK_malloc0(sizeof(NRenderA));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_A;
    r->d.node = node;
    
    r->d.Layout = renderA_layout;
    r->d.Paint = renderA_paint;
    
    return r;
}

void renderA_delete(NRenderA** ra)
{
    NRenderA* r = *ra;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *ra = N_NULL;
}

void renderA_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    rn->needLayout = 0;
}

void renderA_paint(NRenderNode* rn, NStyle* style, NRect* rect)
{
}
