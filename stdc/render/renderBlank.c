/*
 * renderBlank.c
 *
 *  Created on: 2011-5-2
 *      Author: migr
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderBlank_memUsed(const NRenderBlank* rb)
{
    int size = 0;
    if (rb) {
        size = sizeof(NRenderBlank);
    }
    return size;
}
#endif

NRenderBlank* renderBlank_create(void* node)
{
    NRenderBlank* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderBlank*)render_alloc();
#else
	r = (NRenderBlank*)NBK_malloc0(sizeof(NRenderBlank));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_BLANK;
    r->d.node = node;
    
    r->d.Layout = renderBlank_layout;
    r->d.Paint = renderBlank_paint;
    
    return r;
}

void renderBlank_delete(NRenderBlank** render)
{
    NRenderBlank* r = *render;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *render = N_NULL;
}

void renderBlank_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    rn->needLayout = 0;
}

void renderBlank_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
}
