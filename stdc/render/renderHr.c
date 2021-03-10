/*
 * renderHr.c
 *
 *  Created on: 2011-4-4
 *      Author: migr
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderHr_memUsed(const NRenderHr* rh)
{
    int size = 0;
    if (rh) {
        size = sizeof(NRenderHr);
    }
    return size;
}
#endif

NRenderHr* renderHr_create(void* node)
{
    NRenderHr* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderHr*)render_alloc();
#else
	r = (NRenderHr*)NBK_malloc0(sizeof(NRenderHr));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_HR;
    r->d.node = node;

    r->d.Layout = renderHr_layout;
    r->d.Paint = renderHr_paint;
    
    return r;
}

void renderHr_delete(NRenderHr** rh)
{
    NRenderHr* r = *rh;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *rh = N_NULL;
}

void renderHr_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NNode* node = (NNode*)rn->node;
    NRenderHr* rh = (NRenderHr*)rn;
    NAvailArea* aa;
    coord h;
    
    if (!rn->needLayout && !force) {
        layoutStat_rebuildForInline(stat, rn);
        return;
    }
    
    renderNode_calcStyle(rn, style);

    if (!rn->hasBrcolorL) {
        rn->hasBrcolorL = 1;
        rn->border_color_l = colorGray;
    }
    if (!rn->hasBrcolorT) {
        rn->hasBrcolorT = 1;
        rn->border_color_t = rn->border_color_l;
    }
    if (!rn->hasBrcolorR) {
        rn->hasBrcolorR = 1;
        rn->border_color_r = colorLightGray;
    }
    if (!rn->hasBrcolorB) {
        rn->hasBrcolorB = 1;
        rn->border_color_b = rn->border_color_r;
    }

    rh->size = attr_getValueInt32(node->atts, ATTID_SIZE);
    if (rh->size < 0)
        rh->size = 2;
    else if (rh->size < 1)
        rh->size = 1;

    h = rh->size + DEFAULT_LINE_SPACE * 2;

    aa = layoutStat_getAA(stat, rn, h);
    while (!aa->multiline)
        aa = layoutStat_getNextAA(stat, aa, N_TRUE, h);

    rn->r.l = aa->r.l;
    rn->r.t = aa->r.t;
    rn->r.r = aa->r.r;
    rn->r.b = rn->r.t + h;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);
        
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderHr_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderHr* rh = (NRenderHr*)rn;
    coord x, y;
    NRect pr, cl, border;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = rn->r;
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;

    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r);
    cl.b = N_MIN(rect_getHeight(rect), pr.b);
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    pr = rn->r;
    rect_move(&pr, pr.l + x, pr.t + y);
    pr.t += DEFAULT_LINE_SPACE;
    pr.b = pr.t + rh->size;
    rect_toView(&pr, style->zoom);
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    rect_set(&border, 1, 1, 0, 0);
    rect_toView(&border, style->zoom);
    renderNode_drawBorder(rn, &border, &pr, page->platform);

    NBK_gdi_cancelClippingRect(page->platform);
}
