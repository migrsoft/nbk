// create by wuyulun, 2012.2.11

#include "renderInc.h"

NRenderTextPiece* renderTextPiece_create(void)
{
    NRenderTextPiece* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderTextPiece*)render_alloc();
#else
	r = (NRenderTextPiece*)NBK_malloc0(sizeof(NRenderTextPiece));
#endif

    renderNode_init(&r->d);

    r->d.type = RNT_TEXTPIECE;

    r->d.Layout = renderTextPiece_layout;
    r->d.Paint = renderTextPiece_paint;

    return r;
}

void renderTextPiece_delete(NRenderTextPiece** piece)
{
    NRenderTextPiece* r = *piece;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *piece = N_NULL;
}

void renderTextPiece_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    rn->needLayout = 0;
}

void renderTextPiece_paint(NRenderNode* rn, NStyle* style, NRect* rect)
{
}
