/*
 * renderImage.c
 *
 *  Created on: 2010-12-28
 *      Author: wuyulun
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderImage_memUsed(const NRenderImage* ri)
{
    int size = 0;
    if (ri)
        size = sizeof(NRenderImage);
    return size;
}
#endif

NRenderImage* renderImage_create(void* node)
{
    NRenderImage* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderImage*)render_alloc();
#else
	r = (NRenderImage*)NBK_malloc0(sizeof(NRenderImage));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_IMAGE;
    r->d.node = node;
    
    r->d.Layout = renderImage_layout;
    r->d.Paint = renderImage_paint;
    
    r->fail = N_FALSE;
    r->iid = -1;
    
    return r;
}

void renderImage_delete(NRenderImage** ri)
{
    NRenderImage* r = *ri;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *ri = N_NULL;
}

void renderImage_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NRenderImage* ri = (NRenderImage*)rn;
    NNode* node = (NNode*)rn->node;
    coord maxw = renderNode_getContainerWidth(rn);
    coord iw, ih;
    NCssValue* cv;
    NAvailArea* aa;
    
    if (!rn->needLayout && !force) {
        layoutStat_rebuildForInline(stat, rn);
        return;
    }
    
    renderNode_calcStyle(rn, style);
    
    ri->clip = *style->clip;
    
    iw = ih = -1;
    
    cv = attr_getCssValue(node->atts, ATTID_WIDTH);
    if (cv && cv->type)
        iw = css_calcValue(*cv, maxw, style, -1);
    if (iw == -1 && style->width.type)
        iw = css_calcValue(style->width, maxw, style, -1);
    
    cv = attr_getCssValue(node->atts, ATTID_HEIGHT);
    if (cv && cv->type)
        ih = css_calcValue(*cv, 0, style, -1);
    if (ih == -1 && style->height.type)
        ih = css_calcValue(style->height, 0, style, -1);
    
    if ((iw == -1 || ih == -1) && ri->iid != -1 && !ri->fail) {
        NView* view = (NView*)style->view;
        NSize size;
        nbool fail;
        nbool got = imagePlayer_getSize(view->imgPlayer, ri->iid, &size, &fail);
        if (got) {
            iw = size.w;
            ih = size.h;
            ri->fail = N_FALSE;
        }
        else if (fail)
            ri->fail = N_TRUE;
    }
    if (iw == -1)
        iw = UNKNOWN_IMAGE_WIDTH;
    if (ih == -1)
        ih = UNKNOWN_IMAGE_HEIGHT;
    if (iw > maxw) { // keep ratio
        ih = ih * maxw / iw;
        if (!ih)
            ih = 1;
        iw = maxw;
    }

    iw += rn->margin.l + rn->margin.r \
        + rn->border.l + rn->border.r \
        + rn->padding.l + rn->padding.r;
    ih += rn->margin.t + rn->margin.b \
        + rn->border.t + rn->border.b \
        + rn->padding.t + rn->padding.b;
    
    aa = layoutStat_getAA(stat, rn, ih);
    while (!layoutStat_isLastAA(stat, aa) && rect_getWidth(&aa->r) < iw)
        aa = layoutStat_getNextAA(stat, aa, N_TRUE, ih);

    rn->r.l = (rn->flo == CSS_FLOAT_RIGHT) ? aa->r.r - iw : aa->r.l;
    rn->r.t = aa->r.t;
    rn->r.r = rn->r.l + iw;
    rn->r.b = rn->r.t + ih;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);

    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderImage_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderImage* ri = (NRenderImage*)rn;
    coord x, y;
    NRect c, pr, cl;
    
    if (rn->display == CSS_DISPLAY_NONE)
        return;
    
    renderNode_getAbsPos(rn->parent, &x, &y);
    
    pr = rn->r;
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r);
    cl.b = N_MIN(rect_getHeight(rect), pr.b);
    c = ri->clip;
    rect_move(&c, c.l + x, c.t + y);
    rect_toView(&c, style->zoom);
    rect_move(&c, c.l - rect->l, c.t - rect->t);
    cl.r = N_MIN(cl.r, c.r);
    cl.b = N_MIN(cl.b, c.b);
    NBK_gdi_setClippingRect(page->platform, &cl);

    if (renderNode_hasBorder(rn)) {
        NRect border = rn->border;
        rect_toView(&border, style->zoom);
        pr = renderNode_getBorderBox(rn);
        rect_move(&pr, pr.l + x, pr.t + y);
        rect_toView(&pr, style->zoom);
        rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
        renderNode_drawBorder(rn, &border, &pr, page->platform);
    }

    pr = renderNode_getContentBox(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    if (ri->iid == -1 || ri->fail) {
        NBK_gdi_setBrushColor(page->platform, &colorWhite);
        NBK_gdi_fillRect(page->platform, &pr);
        if (!style->highlight) {
            NBK_gdi_setPenColor(page->platform, (rn->flo) ? &colorRed : &colorGray);
            NBK_gdi_drawRect(page->platform, &pr);
        }
    }
    else if (style->drawPic) {
        NView* view = (NView*)style->view;
        imagePlayer_draw(view->imgPlayer, ri->iid, &pr);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
}

int16 renderImage_getDataSize(NRenderNode* rn, NStyle* style)
{
    NRenderImage* ri = (NRenderImage*)rn;
    NNode* node = (NNode*)rn->node;
    coord iw = -1;
    int16 size;
    NCssValue* cv;
    NStyle sty;

    cv = attr_getCssValue(node->atts, ATTID_WIDTH);
    if (cv && cv->type)
        iw = css_calcValue(*cv, 0, style, -1);
    
    if (iw == -1) {
        style_init(&sty);
        node_calcStyle(style->view, node, &sty);
        if (sty.width.type)
            iw = css_calcValue(sty.width, 0, &sty, -1);
    }

    if (iw == -1 && ri->iid != -1) {
        NView* view = (NView*)style->view;
        NSize size;
        nbool fail;
        if (imagePlayer_getSize(view->imgPlayer, ri->iid, &size, &fail))
            iw = size.w;
    }

    if (iw == -1)
        iw = UNKNOWN_IMAGE_WIDTH;
    
    size = iw / style->font_size;
    if (iw % style->font_size)
        size++;
    
    return size;
}
