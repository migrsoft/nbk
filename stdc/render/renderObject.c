/*
 * renderObject.c
 *
 *  Created on: 2011-4-26
 *      Author: wuyulun
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderObject_memUsed(const NRenderObject* ro)
{
    int size = 0;
    if (ro)
        size = sizeof(NRenderObject);
    return size;
}
#endif

NRenderObject* renderObject_create(void* node)
{
    NRenderObject* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderObject*)render_alloc();
#else
	r = (NRenderObject*)NBK_malloc0(sizeof(NRenderObject));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_OBJECT;
    r->d.node = node;
    
    r->d.Layout = renderObject_layout;
    r->d.Paint = renderObject_paint;
    
    r->iid = -1;
    
    return r;
}

void renderObject_delete(NRenderObject** object)
{
    NRenderObject* r = *object;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *object = N_NULL;
}

void renderObject_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    if (!rn->needLayout && !force)
        return;

    rn->display = CSS_DISPLAY_NONE;
    
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderObject_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
}
