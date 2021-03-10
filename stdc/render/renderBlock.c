/*
 * renderBlock.c
 *
 *  Created on: 2010-12-29
 *      Author: wuyulun
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderBlock_memUsed(const NRenderBlock* rb)
{
    int size = 0;
    if (rb) {
        size = sizeof(NRenderBlock);
    }
    return size;
}
#endif

NRenderBlock* renderBlock_create(void* node)
{
    NRenderBlock* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderBlock*)render_alloc();
#else
	r = (NRenderBlock*)NBK_malloc0(sizeof(NRenderBlock));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_BLOCK;
    r->d.node = node;

    r->d.Layout = renderBlock_layout;
    r->d.Paint = renderBlock_paint;
    
    return r;
}

void renderBlock_delete(NRenderBlock** block)
{
    NRenderBlock* r = *block;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *block = N_NULL;
}

static void block_align_line(NRenderNode* begin, NRenderNode* end, coord dx, coord maxb)
{
    NRenderNode* r = begin;
    coord dy;
    
    while (r && r != end) {
        if (   r->display
            && !r->flo
            && r->type != RNT_BR
            && r->type != RNT_BLOCK )
        {
            r->r.l = r->og_r.l + dx;
            r->r.r = r->og_r.r + dx;
            
            dy = calcVertAlignDelta(maxb, r->og_r.b, r->vert_align);
            r->r.t = r->og_r.t + dy;
            r->r.b = r->og_r.b + dy;

            if (r->type == RNT_TEXTPIECE) {
                r->parent->r.r = N_MAX(r->parent->r.r, r->r.r);
                r->parent->r.b = N_MAX(r->parent->r.b, r->r.b);
            }
        }
        r = renderNode_getNextNode(r);
    }
}

void block_align(NRenderNode* rn, nid ha)
{
    NRenderNode *b, *r;
    coord w, ml, mt, mr, mb;
    coord maxw = renderNode_getInnerWidth(rn);
    nbool found;
    
    b = N_NULL;
    r = renderNode_getFirstChild(rn);
    ml = N_MAX_COORD;
    mr = maxw;
    w = mb = 0;
    mt = -1;
    found = N_FALSE;
    while (r) {
        if (r->display && !r->flo && r->type != RNT_BR) {
            if (b == N_NULL)
                b = r;
            if (mt == -1)
                mt = r->og_r.t;
            
            if (mt == r->og_r.t) {
                // at same line
                w += rect_getWidth(&r->og_r);
                ml = N_MIN(ml, r->og_r.l);
                mb = N_MAX(mb, r->og_r.b);
                mr = N_MIN(mr, r->aa_r.r);
            }
            else
                found = N_TRUE;
            
            if (found) {
                found = N_FALSE;
                
                block_align_line(b, r, calcHoriAlignDelta(mr - ml, w, ha), mb);

                b = r;
                w = rect_getWidth(&r->og_r);
                mt = r->og_r.t;
                ml = r->og_r.l;
                mb = r->og_r.b;
                mr = N_MIN(maxw, r->aa_r.r);
            }
        }
        
        r = renderNode_getNextNode(r);
    }
    
    if (b)
        block_align_line(b, N_NULL, calcHoriAlignDelta(mr - ml, w, ha), mb);
}

// margin overlay
//static coord block_calc_y_with_overlay(NRenderNode* rn, coord y)
//{
//    coord yy = y;
//    if (   rn->prev && rn->prev->type == RNT_BLOCK && rn->display
//        && (rn->clr || rn->prev->flo == CSS_FLOAT_NONE)) {
//        // margin overlay
//		coord delta = N_MIN(rn->prev->margin.b, rn->margin.t);
//        yy -= delta;
//    }
//    return yy;
//}

void renderBlock_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NRenderNode* r;
    NRenderBlock* rb = (NRenderBlock*)rn;
    NNode* n;
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
        layoutStat_rebuildForBlock(stat, rn);
        layoutStat_uniteAA(stat, rn);
        return;
    }
    
    /*
     * layout:
     * 1. initializate width
     * 2. layout child
     *    adjust pos & size
     */
    
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
        
        rb->hasWidth = rb->hasHeight = 0;
        rb->text_align = style->text_align;
        rb->overflow = style->overflow;
        
        n = (NNode*)rn->node;
        rb->ne_display = (attr_getValueStr(n->atts, ATTID_NE_DISPLAY)) ? 1 : 0;

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
        
        // 忽略百分比形式的高度设定（不能确定实际高度值）
        th = 0;
        if (style->height.type && style->height.type != NECVT_PERCENT) {
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

    // 计算最大区域
    r = renderNode_getFirstChild(rn);
    th = 0;
    rb->overflow_h = 0;
    while (r) {
        if (r->display) {
            rb->overflow_h = N_MAX(rb->overflow_h, r->r.b);
            if (!r->flo)
                th = N_MAX(th, r->r.b);
        }
        r = renderNode_getNextNode(r);
    }
    
    if (rb->hasWidth || !rn->parent)
        tw = rect_getWidth(&rn->r);
    else
        tw = renderNode_getContainerWidth(rn);
    
    if (rb->hasHeight)
        th = rect_getHeight(&rn->r);
    else {
        if (rn->flo || rb->overflow == CSS_OVERFLOW_HIDDEN)
            th = rb->overflow_h;
        th += rn->margin.t + rn->margin.b \
            + rn->border.t + rn->border.b \
            + rn->padding.t + rn->padding.b;
    }

    rb->overflow_h -= th;
    
    aa = layoutStat_getAA(stat, rn, 0);

    rn->r.l = 0;
    rn->r.t = aa->r.t;
    rn->r.r = tw;
    rn->r.b = rn->r.t + th;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);
    layoutStat_uniteAA(stat, rn);

    block_align(rn, rb->text_align);
    
    rn->init = 0;
    rn->force = 0;
    rn->needLayout = 0;
    if (rn->parent)
        rn->parent->needLayout = 1;
}

void renderBlock_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    coord x, y;
    NRect pr, cl, border;
    
    if (rn->display == CSS_DISPLAY_NONE)
        return;
    
    renderNode_getAbsPos(rn->parent, &x, &y);
    
    pr = rn->r;
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    if (style->ne_fold)
        NBK_gdi_setBrushColor(page->platform, &style->background_color);
    else if (rn->hasBgcolor)
        NBK_gdi_setBrushColor(page->platform, &rn->background_color);
        
    pr.l = x + rn->r.l + rn->margin.l;
    pr.t = y + rn->r.t + rn->margin.t;
    pr.r = pr.l + rect_getWidth(&rn->r) - rn->margin.l - rn->margin.r;
    pr.b = pr.t + rect_getHeight(&rn->r) - rn->margin.t - rn->margin.b;
    rect_toView(&pr, style->zoom);
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r);
    cl.b = N_MIN(rect_getHeight(rect), pr.b);
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (rn->hasBgcolor && !style->highlight) {
        if (rn->parent)
            NBK_gdi_fillRect(page->platform, &pr);
        else {
            pr = rn->r;
            rect_toView(&pr, style->zoom);
            NBK_gdi_fillRect(page->platform, &pr);
        }
    }
    
    border = rn->border;
    rect_toView(&border, style->zoom);
    renderNode_drawBorder(rn, &border, &pr, page->platform);
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (rn->bgiid != -1 && !style->highlight) {
        pr = rn->r;
        pr.l += rn->margin.l + rn->border.l;
        pr.t += rn->margin.t + rn->border.t;
        pr.r -= rn->margin.r + rn->border.r;
        pr.b -= rn->margin.b + rn->border.b;
        rect_move(&pr, pr.l + x, pr.t + y);
        rect_toView(&pr, style->zoom);
        rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
        renderNode_drawBgImage(rn, &pr, rect, style);
    }
}
