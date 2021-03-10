/*
 * renderTr.c
 *
 *  Created on: 2011-9-9
 *      Author: wuyulun
 */

#include "renderInc.h"

NRenderTr* renderTr_create(void* node)
{
    NRenderTr* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderTr*)render_alloc();
#else
	r = (NRenderTr*)NBK_malloc0(sizeof(NRenderTr));
#endif
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_TR;
    r->d.node = node;

    r->d.Layout = renderTr_layout;
    r->d.Paint = renderTr_paint;
    
    return r;
}

void renderTr_delete(NRenderTr** rt)
{
    NRenderTr* r = *rt;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *rt = N_NULL;
}

void renderTr_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NRenderTr* rt = (NRenderTr*)rn;
    NRenderNode* r;
    coord tw, th;
    NAvailArea* aa;
    
    if (!rn->needLayout && !force) {
        if (!rn->init) {
            rn->init = 1;
            layoutStat_push(stat, rn);
            layoutStat_init(stat, N_MAX_COORD);
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
        
        renderNode_calcStyle(rn, style);

        rn->flo = CSS_FLOAT_NONE;
        
        tw = renderNode_getContainerWidth(rn);

        rn->r.l = 0;
        rn->r.r = tw;

        layoutStat_push(stat, rn);
        layoutStat_init(stat, N_MAX_COORD);

        if (rn->child)
            return;
    }

    layoutStat_pop(stat);

    r = rn->child;
    tw = th = 0;
    while (r) {
        if (r->display && r->type == RNT_TD) {
            tw = N_MAX(tw, r->r.r);
            th = N_MAX(th, r->r.b);
        }
        r = r->next;
    }
    
    // adjust all cells to max height
    r = rn->child;
    while (r) {
        if (r->display && r->type == RNT_TD)
            r->r.b = r->r.t + th;
        r = r->next;
    }

    aa = layoutStat_getAA(stat, rn, 0);
    while (!aa->multiline /*|| rect_getWidth(&aa->r) < tw*/)
        aa = layoutStat_getNextAA(stat, aa, N_FALSE, 0);

    rn->r.l = aa->r.l;
    rn->r.t = aa->r.t;
    rn->r.r = rn->r.l + tw;
    rn->r.b = rn->r.t + th;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);

    rn->init = 0;
    rn->force = 0;
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderTr_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    coord x, y;
    NRect pr, cl;
    
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
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (rn->hasBgcolor) {
        NBK_gdi_setBrushColor(page->platform, &rn->background_color);
        NBK_gdi_fillRect(page->platform, &pr);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (rn->bgiid != -1 && !style->highlight) {
        renderNode_drawBgImage(rn, &pr, rect, style);
    }
}

int renderTr_getColumnNum(NRenderNode* rn)
{
    NRenderNode* r = rn->child;
    int n = 0;
    
    while (r) {
        if (r->type == RNT_TD)
            n++;
        r = r->next;
    }
    return n;
}

void renderTr_predictColsWidth(NRenderNode* rn, int16* colsWidth, int maxCols, NStyle* style)
{
    NRenderNode* r = rn->child;
    int i = 0;
//    NPage* page = (NPage*)((NView*)style->view)->page;
    
    while (r) {
        if (r->type == RNT_TD) {
            int16 size = renderTd_getDataSize(r, style);
            if (i == maxCols - 1)
                break;
//            dump_int(page, i);
//            dump_int(page, size);
//            dump_return(page);
            colsWidth[i] = N_MAX(size, colsWidth[i]);
            i++;
        }
        r = r->next;
    }
}

coord renderTr_getContentWidthByTd(NRenderNode* tr, NRenderNode* td)
{
    NRenderTable* table = (NRenderTable*)renderNode_getParent(tr->parent, RNT_TABLE);
    NRenderNode* r;
    int16 i;
    
    r = tr->child;
    i = 0;
    while (r) {
        if (r->type == RNT_TD) {
            if (r == td) {
                return renderTable_getColWidth(table, i);
            }
            i++;
        }
        r = r->next;
    }
    
    return 0;
}
