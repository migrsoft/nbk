/*
 * renderInline.c
 *
 *  Created on: 2011-10-31
 *      Author: wuyulun
 */

#include "renderInc.h"

NRenderInline* renderInline_create(void* node)
{
    NRenderInline* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderInline*)render_alloc();
#else
	r = (NRenderInline*)NBK_malloc0(sizeof(NRenderInline));
#endif
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_INLINE;
    r->d.node = node;
    
    r->d.Layout = renderInline_layout;
    r->d.Paint = renderInline_paint;
    
    return r;
}

void renderInline_delete(NRenderInline** ri)
{
    NRenderInline* r = *ri;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *ri = N_NULL;
}

static void layout_holder(NLayoutStat* stat, NRenderNode* rn, coord w, coord h)
{
    NAvailArea* aa;

    aa = layoutStat_getAA(stat, rn, h);
    while (rect_getWidth(&aa->r) < w)
        aa = layoutStat_getNextAA(stat, aa, N_TRUE, h);

    rn->r.l = aa->r.l;
    rn->r.t = aa->r.t;
    rn->r.r = rn->r.l + w;
    rn->r.b = rn->r.t + h;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    rn->needLayout = 0;

    layoutStat_updateAA(stat, rn, aa);
}

void renderInline_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderInline* ri = (NRenderInline*)rn;
    NRenderTextPiece* rtp;
    coord maxw, w, h;

    if (!rn->needLayout && !force) {
        if (!rn->init) {
            rn->init = 1;
            if (   rn->child
                && rn->child->type == RNT_TEXTPIECE
                && ((NRenderTextPiece*)rn->child)->type == NETPIE_HOLDER_BEGIN )
                layoutStat_rebuildForInline(stat, rn->child);
            if (rn->child)
                return;
        }
        rn->init = 0;
        if (rn->child) {
            rtp = (NRenderTextPiece*)rtree_get_last_sibling(rn->child);
            if (   rtp->d.type == RNT_TEXTPIECE
                && rtp->type == NETPIE_HOLDER_END )
                layoutStat_rebuildForInline(stat, (NRenderNode*)rtp);
        }
        return;
    }

    if (force) {
        if (rn->force)
            rn->force = 0;
        else {
            rn->force = 1;
            rn->init = 0;
        }
    }

    if (!rn->init) {
        rn->init = 1;

        renderNode_calcStyle(rn, style);

        ri->lh = style->font_size;

        maxw = renderNode_getContainerWidth(rn);

        w = 0;
        if (style->width.type) {
            ri->hasWidth = 1;
            w = css_calcValue(style->width, maxw, style, maxw);
        }
        else
            ri->hasWidth = 0;

        h = 0;
        if (style->height.type) {
            ri->hasHeight = 1;
            h = css_calcValue(style->height, 0, style, 0);
        }
        else
            ri->hasHeight = 0;

        rn->r.r = w;
        rn->r.b = h;

        if (rn->margin.l || rn->border.l || rn->padding.l) {
            if (rn->child == N_NULL || rn->child->type != RNT_TEXTPIECE) {
                rtp = renderTextPiece_create();
                rtp->d.parent = rn;
                rtp->type = NETPIE_HOLDER_BEGIN;
                if (rn->child == N_NULL) {
                    rn->child = (NRenderNode*)rtp;
                }
                else {
                    rtp->d.next = rn->child;
                    rn->child->prev = (NRenderNode*)rtp;
                    rn->child = (NRenderNode*)rtp;
                }
            }
            else
                rtp = (NRenderTextPiece*)rn->child;

            w = rn->margin.l + rn->border.l + rn->padding.l;

            layout_holder(stat, (NRenderNode*)rtp, w, ri->lh);
        }

        if (rn->child)
            return;
    }

    if (rn->margin.r || rn->border.r || rn->padding.r) {
        NRenderNode* r = N_NULL;

        if (rn->child)
            r = rtree_get_last_sibling(rn->child);

        if (    r == N_NULL
            ||  r->type != RNT_TEXTPIECE
            || (r->type == RNT_TEXTPIECE && ((NRenderTextPiece*)r)->type != NETPIE_HOLDER_END) )
        {
            rtp = renderTextPiece_create();
            rtp->d.parent = rn;
            rtp->type = NETPIE_HOLDER_END;
            if (r == N_NULL) {
                rn->child = (NRenderNode*)rtp;
            }
            else {
                rtp->d.prev = r;
                r->next = (NRenderNode*)rtp;
            }
        }
        else
            rtp = (NRenderTextPiece*)r;

        w = rn->margin.r + rn->border.r + rn->padding.r;

        layout_holder(stat, (NRenderNode*)rtp, w, ri->lh);
    }

    rn->init = 0;
    rn->force = 0;
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderInline_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
}
