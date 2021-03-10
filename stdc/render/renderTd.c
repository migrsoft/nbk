/*
 * renderTd.c
 *
 *  Created on: 2011-9-9
 *      Author: wuyulun
 */

#include "renderInc.h"

NRenderTd* renderTd_create(void* node)
{
    NRenderTd* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderTd*)render_alloc();
#else
	r = (NRenderTd*)NBK_malloc0(sizeof(NRenderTd));
#endif
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_TD;
    r->d.node = node;
    
    r->d.Layout = renderTd_layout;
    r->d.Paint = renderTd_paint;
    
    return r;
}

void renderTd_delete(NRenderTd** rt)
{
    NRenderTd* r = *rt;
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *rt = N_NULL;
}

void renderTd_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NRenderTd* rt = (NRenderTd*)rn;
    NRenderNode* r;
    coord tw, th;
    NAvailArea* aa;

    if (!rn->needLayout && !force) {
        if (!rn->init) {
            rn->init = 1;
            layoutStat_push(stat, rn);
            layoutStat_init(stat, rt->aa_width);
            if (rn->child)
                return;
        }
        rn->init = 0;
        layoutStat_pop(stat);
        layoutStat_rebuildForBlock(stat, rn);
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
        rt->text_align = style->text_align;

        r = renderNode_getParent(rn->parent, RNT_TABLE);

        rn->border = r->border;
        if (rn->border.l > 0) {
            if (!rn->hasBrcolorL) {
                rn->hasBrcolorL = 1;
                rn->border_color_l = colorBlack;
            }
            if (!rn->hasBrcolorT) {
                rn->hasBrcolorT = 1;
                rn->border_color_t = colorBlack;
            }
            if (!rn->hasBrcolorR) {
                rn->hasBrcolorR = 1;
                rn->border_color_r = colorGray;
            }
            if (!rn->hasBrcolorB) {
                rn->hasBrcolorB = 1;
                rn->border_color_b = colorGray;
            }
        }

        rn->padding.l = rn->padding.t = rn->padding.r = rn->padding.b = ((NRenderTable*)r)->cellpadding;

        tw = renderTr_getContentWidthByTd(rn->parent, rn);
        tw -= rn->border.l * 2 + rn->padding.l * 2;

        rn->r.l = 0;
        rn->r.r = tw;

        rt->aa_width = renderNode_getInnerWidth(rn);
        layoutStat_push(stat, rn);
        layoutStat_init(stat, rt->aa_width);

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
        }
        r = renderNode_getNextNode(r);
    }
    
    tw += rn->border.l * 2 + rn->padding.l * 2;
    th += rn->border.t * 2 + rn->padding.t * 2;

    aa = layoutStat_getAA(stat, rn, th);

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

void renderTd_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    coord x, y;
    NRect pr, cl;
    NRenderTable* table = (NRenderTable*)renderNode_getParent(rn->parent, RNT_TABLE);
    
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

    if (table->d.border.l > 0) {
        NRect border = rn->border;
        rect_toView(&border, style->zoom);
        renderNode_drawBorder(rn, &border, &pr, page->platform);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
}

typedef struct _NTdGetSizeTask {
    NStyle* style;
    int     size;
    int     text;
    nbool    hasText;
} NTdGetSizeTask;

static int td_get_size_cb(NRenderNode* render, void* user, nbool* ignore)
{
    NTdGetSizeTask* task = (NTdGetSizeTask*)user;
    
    switch (render->type) {
    case RNT_TEXT:
        task->text += renderText_getDataSize(render, task->style);
        task->hasText = N_TRUE;
        break;
    case RNT_IMAGE:
        task->size += renderImage_getDataSize(render, task->style);
        break;
    case RNT_INPUT:
        task->size += renderInput_getDataSize(render, task->style);
        break;
    case RNT_SELECT:
        task->size += renderSelect_getDataSize(render, task->style);
        break;
    case RNT_TEXTAREA:
        task->size += renderTextarea_getDataSize(render, task->style);
        break;
    }
    
    return 0;
}

int16 renderTd_getDataSize(NRenderNode* rn, NStyle* style)
{
    NTdGetSizeTask task;
    task.style = style;
    task.size = 0;
    task.text = 0;
    task.hasText = N_FALSE;
    rtree_traverse_depth(rn, td_get_size_cb, N_NULL, &task);
    
    if (task.hasText) {
        if (task.text < 10)
            task.size += task.text;
        else
            task.size += 10;
    }
    
    return (int16)task.size;
}
