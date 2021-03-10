// create by wuyulun, 2012.2.16

#include "renderInc.h"

NRenderInlineBlock* renderInlineBlock_create(void* node)
{
    NRenderInlineBlock* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderInlineBlock*)render_alloc();
#else
	r = (NRenderInlineBlock*)NBK_malloc0(sizeof(NRenderInlineBlock));
#endif

    renderNode_init(&r->d);
    
    r->d.type = RNT_INLINE_BLOCK;
    r->d.node = node;

    r->d.Layout = renderInlineBlock_layout;
    r->d.Paint = renderInlineBlock_paint;

    return r;
}

void renderInlineBlock_delete(NRenderInlineBlock** iblock)
{
    NRenderInlineBlock* r = *iblock;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *iblock = N_NULL;
}

void renderInlineBlock_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NRenderNode* r;
    NRenderInlineBlock* rb = (NRenderInlineBlock*)rn;
    coord tw, th;
    NAvailArea* aa;

    if (!rn->needLayout && !force) {
        if (!rn->init) {
            rn->init = 1;
            layoutStat_push(stat, rn);
            layoutStat_init(stat, rb->aa_width);
            if (rn->child)
                return;
        }
        rn->init = 0;
        layoutStat_pop(stat);
        layoutStat_rebuildForInline(stat, rn);
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
        rb->hasWidth = rb->hasHeight = 0;
        
        renderNode_calcStyle(rn, style);
        
        rb->text_align = style->text_align;
        rb->overflow = style->overflow;
        
        if (rn->parent)
            tw = renderNode_getContainerWidth(rn);
        else
            tw = rect_getWidth(&rn->r);

        if (style->width.type) {
            rb->hasWidth = 1;
            tw = css_calcValue(style->width, tw, style, tw);
        }
        else
            rb->hasWidth = 0;
        
        th = 0;
        if (style->height.type) {
            rb->hasHeight = 1;
            th = css_calcValue(style->height, th, style, th);
        }
        else
            rb->hasHeight = 0;
        
        if (rb->overflow == CSS_OVERFLOW_HIDDEN && tw && th)
            rect_set(style->clip, 0, 0, tw, th);
        
        if (rb->hasWidth)
            tw += rn->margin.l + rn->margin.r \
                + rn->border.l + rn->border.r \
                + rn->padding.l + rn->padding.r;
        
        if (rb->hasHeight)
            th += rn->margin.t + rn->margin.b \
                + rn->border.t + rn->border.b \
                + rn->padding.t + rn->padding.b;

        rn->r.l = 0;
        rn->r.r = tw;
        rn->r.t = 0;
        rn->r.b = th;

        rb->aa_width = renderNode_getInnerWidth(rn);
        layoutStat_push(stat, rn);
        layoutStat_init(stat, rb->aa_width);

        if (rn->child)
            return;
    }

    layoutStat_pop(stat);

    r = renderNode_getFirstChild(rn);
    tw = th = 0;
    while (r) {
        if (r->display) {
            tw = N_MAX(tw, r->r.r);
            th = N_MAX(th, r->r.b);
            if (r->type == RNT_BLOCK)
                th = N_MAX(th, ((NRenderBlock*)r)->overflow_h);
        }
        r = renderNode_getNextNode(r);
    }
    
    if (rb->hasWidth)
        tw = rect_getWidth(&rn->r);
    else
        tw += rn->margin.l + rn->margin.r \
            + rn->border.l + rn->border.r \
            + rn->padding.l + rn->padding.r;
    
    if (rb->hasHeight)
        th = rect_getHeight(&rn->r);
    else
        th += rn->margin.t + rn->margin.b \
            + rn->border.t + rn->border.b \
            + rn->padding.t + rn->padding.b;
    
    aa = layoutStat_getAA(stat, rn, th);
    while (!layoutStat_isLastAA(stat, aa) && rect_getWidth(&aa->r) < tw)
        aa = layoutStat_getNextAA(stat, aa, N_TRUE, th);

    rn->r.l = (rn->flo == CSS_FLOAT_RIGHT) ? aa->r.r - tw : aa->r.l;
    rn->r.t = aa->r.t;
    rn->r.r = rn->r.l + tw;
    rn->r.b = rn->r.t + th;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);

    block_align(rn, rb->text_align);
    
    rn->init = 0;
    rn->force = 0;
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderInlineBlock_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    renderBlock_paint(rn, style, rect);
}
