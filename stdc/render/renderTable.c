/*
 * renderTable.c
 *
 *  Created on: 2011-9-9
 *      Author: wuyulun
 */

#include "renderInc.h"

#define DUMP_FUNCTIONS      1
#define NBK_TABLE_MAX_COLS  8

NRenderTable* renderTable_create(void* node)
{
    NRenderTable* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderTable*)render_alloc();
#else
	r = (NRenderTable*)NBK_malloc0(sizeof(NRenderTable));
#endif
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_TABLE;
    r->d.node = node;
    
    r->d.Layout = renderTable_layout;
    r->d.Paint = renderTable_paint;
    
    return r;
}

void renderTable_delete(NRenderTable** rt)
{
    NRenderTable* r = *rt;
    if (r->cols_width)
        NBK_free(r->cols_width);
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *rt = N_NULL;
}

#if DUMP_FUNCTIONS
static void table_dump_cols_width(NRenderTable* table, NStyle* style)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    int i;
    
    dump_char(page, "table column width", -1);
    dump_return(page);
    for (i=0; table->cols_width[i] != -1; i++) {
        dump_int(page, i);
        dump_int(page, table->cols_width[i]);
        dump_return(page);
    }
    dump_flush(page);
}
#endif

static int table_max_column(NRenderNode* rn)
{
    NRenderNode* r = rn->child;
    int max = 0;
    
    while (r) {
        if (r->type == RNT_TR)
            max = N_MAX(max, renderTr_getColumnNum(r));
        r = r->next;
    }
    return max;
}

static nbool table_has_design_width(NRenderTable* table, coord maxw, NStyle* style)
{
    NCssValue *cvs, *cv;
    NRenderNode* r;
    nbool set;
    int i, j, n;
    coord w, used;
    
    r = ((NRenderNode*)table)->child;
    while (r) {
        if (r->type == RNT_TR)
            break;
        r = r->next;
    }
    if (r == N_NULL)
        return N_FALSE;
    
    r = r->child;
    while (r) {
        if (r->type == RNT_TD)
            break;
        r = r->next;
    }
    if (r == N_NULL)
        return N_FALSE;
    
    cvs = (NCssValue*)NBK_malloc0(sizeof(NCssValue) * table->cols_num);
    set = N_FALSE;
    i = -1;
    while (r) {
        if (r->type == RNT_TD) {
            i++;
            cv = attr_getCssValue(((NNode*)r->node)->atts, ATTID_WIDTH);
            if (cv && cv->type) {
                cvs[i] = *cv;
                set = N_TRUE;
            }
        }
        r = r->next;
    }
    n = i;
    
    if (set) {
        maxw -= table->d.padding.l * i;
        used = 0;
        for (i=j=0; i <= n; i++) {
            if (cvs[i].type) {
                w = css_calcValue(cvs[i], maxw, style, 0);
                table->cols_width[i] = w;
                used += w;
            }
            else
                j++;
        }
        // average allocate rest space for columns
        if (j) {
            w = (maxw - used) / j;
            if (w < 0) w = 0;
            for (i=0; i <= n; i++) {
                if (cvs[i].type == NECVT_NONE)
                    table->cols_width[i] = w;
            }
        }
    }
    
    NBK_free(cvs);
    return set;
}

static void table_predict_column_width(NRenderTable* table, coord maxw, NStyle* style)
{
    NRenderNode* r;
    int i, num;
    
    num = table_max_column((NRenderNode*)table);
    if (num < NBK_TABLE_MAX_COLS)
        num = NBK_TABLE_MAX_COLS;
    
    if (table->cols_num < num) {
        if (table->cols_width)
            NBK_free(table->cols_width);
        table->cols_width = (int16*)NBK_malloc(sizeof(int16) * (num + 1));
        table->cols_num = num;
    }
    NBK_memset(table->cols_width, -1, sizeof(int16) * (table->cols_num + 1));
    
    if (table_has_design_width(table, maxw, style))
        return;
    
    r = ((NRenderNode*)table)->child;
    while (r) {
        if (r->type == RNT_TR) {
            renderTr_predictColsWidth(r, table->cols_width, table->cols_num, style);
        }
        r = r->next;
    }
    
    for (i=num=0; table->cols_width[i] != -1; i++)
        num += table->cols_width[i];
    
    if (num == 0)
        return;
    
    maxw -= table->d.padding.l * (i - 1);
    
    //table_dump_cols_width(table, style);
    
    for (i=0; table->cols_width[i] != -1; i++)
        table->cols_width[i] = table->cols_width[i] * maxw / num;
    
    //table_dump_cols_width(table, style);
}

static void table_adjust_column(NRenderTable* table, NStyle* style, nbool shrink)
{
    NRenderNode *tr, *td;
    int row, col;
    coord x, y, h, spacing;
    
    if (shrink) { // when width set to auto
        NBK_memset(table->cols_width, -1, sizeof(int16) * (table->cols_num + 1));
        tr = ((NRenderNode*)table)->child;
        while (tr) {
            if (tr->type == RNT_TR) {
                td = tr->child;
                col = 0;
                while (td && col < table->cols_num) {
                    if (td->type == RNT_TD) {
                        table->cols_width[col] = N_MAX(table->cols_width[col], rect_getWidth(&td->r));
                        col++;
                    }
                    td = td->next;
                }
            }
            tr = tr->next;
        }
    }
    
//    table_dump_cols_width(table, style);
    
    spacing = (table->d.padding.l > 0) ? table->d.padding.l : 0;
    row = 0;
    y = -1;
    tr = ((NRenderNode*)table)->child;
    while (tr) {
        if (tr->type == RNT_TR) {
            if (y == -1)
                y = tr->og_r.t;

            h = rect_getHeight(&tr->og_r);
            if (row && spacing) {
                y += spacing;
                tr->r.t = y;
                tr->r.b = y + h;
            }

            td = tr->child;
            col = x = 0;
            while (td && col < table->cols_num) {
                if (td->type == RNT_TD) {
                    td->r.l = x;
                    td->r.r = x + table->cols_width[col];
                    //block_align(td, ((NRenderTd*)td)->text_align);
                    tr->r.r = N_MAX(tr->r.r, td->r.r);
                    x = td->r.r + spacing;
                    col++;
                }
                td = td->next;
            }

            y += h;
            row++;
        }
        tr = tr->next;
    }
}

static int table_is_changed_worker(NRenderNode* rn, void* user, nbool* ignore)
{
    nbool* change = (nbool*)user;
    
    switch(rn->type) {
    case RNT_TABLE:
    case RNT_TR:
    case RNT_TD:
        if (rn->needLayout) {
            *change = N_TRUE;
            return 1;
        }
        break;
    default:
        break;
    }
    
    return 0;
}

static nbool table_is_changed(NRenderNode* rn)
{
    nbool change = N_FALSE;
    rtree_traverse_depth(rn, table_is_changed_worker, N_NULL, &change);
    return change;
}

static int table_relayout_worker(NRenderNode* rn, void* user, nbool* ignore)
{
    rn->init = 0;
    rn->force = 0;
    rn->needLayout = 1;
    return 0;
}

static void table_relayout(NRenderNode* rn)
{
    rtree_traverse_depth(rn, table_relayout_worker, N_NULL, N_NULL);
}

void renderTable_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NRenderTable* rt = (NRenderTable*)rn;
    NNode* node = (NNode*)rn->node;
    NRenderNode* r;
    coord tw, th, border, cellspacing;
    NCssValue* cv;
    NAvailArea* aa;
    
    if (!rn->needLayout && !force) {
        if (table_is_changed(rn))
            table_relayout(rn);
    }
    
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
        rt->hasWidth = 0;
        
        renderNode_calcStyle(rn, style);
        
        border = attr_getCoord(node->atts, ATTID_BORDER);
        if (border != -1) {
            rn->border.l = rn->border.t = rn->border.r = rn->border.b = border;
            if (!rn->hasBrcolorL) {
                rn->hasBrcolorL = 1;
                rn->border_color_l = colorGray;
            }
            if (!rn->hasBrcolorT) {
                rn->hasBrcolorT = 1;
                rn->border_color_t = colorGray;
            }
            if (!rn->hasBrcolorR) {
                rn->hasBrcolorR = 1;
                rn->border_color_r = colorBlack;
            }
            if (!rn->hasBrcolorB) {
                rn->hasBrcolorB = 1;
                rn->border_color_b = colorBlack;
            }
        }

        cellspacing = attr_getCoord(node->atts, ATTID_CELLSPACING);
        if (cellspacing > 0)
            rn->padding.l = rn->padding.t = rn->padding.r = rn->padding.b = cellspacing;

        rt->cellpadding = attr_getCoord(node->atts, ATTID_CELLPADDING);
        if (rt->cellpadding < 0)
            rt->cellpadding = 1;
        
        tw = renderNode_getContainerWidth(rn);
        cv = attr_getCssValue(node->atts, ATTID_WIDTH);
        if (cv && cv->type) {
            tw = css_calcValue(*cv, tw, style, tw);
            rt->hasWidth = 1;
        }
        else if (style->width.type) {
            tw = css_calcValue(style->width, tw, style, tw);
            rt->hasWidth = 1;
        }
        
        if (rt->hasWidth)
            tw += rn->margin.l + rn->margin.r + rn->border.l * 2 + rn->padding.l * 2;

        rn->r.l = 0;
        rn->r.r = tw;
        
        if (!rt->hasWidth) {
            tw -= rn->margin.l + rn->margin.r + rn->border.l * 2 + rn->padding.l * 2;
            if (rn->padding.l == 0)
                tw += rn->border.l * 2;
        }

        table_predict_column_width(rt, tw, style);

        rt->aa_width = renderNode_getInnerWidth(rn);
        layoutStat_push(stat, rn);
        layoutStat_init(stat, rt->aa_width);

        if (rn->child)
            return;
    }

    layoutStat_pop(stat);

    table_adjust_column(rt, style, (rt->hasWidth) ? N_FALSE : N_TRUE);
    
    r = rn->child;
    tw = th = 0;
    while (r) {
        if (r->display && r->type == RNT_TR) {
            tw = N_MAX(tw, r->r.r);
            th = N_MAX(th, r->r.b);
        }
        r = r->next;
    }
    
    tw += rn->margin.l + rn->margin.r + rn->border.l * 2 + rn->padding.l * 2;
    // table may be grow
    if (rt->hasWidth)
        tw = N_MAX(tw, rect_getWidth(&rn->r));

    th += rn->margin.t + rn->margin.b + rn->border.l * 2 + rn->padding.l * 2;

    aa = layoutStat_getAA(stat, rn, 0);
    if (rn->flo) {
        while (rect_getWidth(&aa->r) < tw)
            aa = layoutStat_getNextAA(stat, aa, N_FALSE, 0);
        rn->r.l = (rn->flo == CSS_FLOAT_RIGHT) ? aa->r.r - tw : aa->r.l;
        rn->r.t = aa->r.t;
        rn->r.r = rn->r.l + tw;
        rn->r.b = rn->r.t + th;
    }
    else {
        rn->r.l = aa->r.l;
        rn->r.t = aa->r.t;
        rn->r.r = rn->r.l + tw;
        rn->r.b = rn->r.t + th;
    }
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);

    rn->init = 0;
    rn->force = 0;
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderTable_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    coord x, y;
    NRect pr, cl;
    
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
    
    if (rn->border.l > 0) {
        NRect border = rn->border;
        rect_toView(&border, style->zoom);
        renderNode_drawBorder(rn, &border, &pr, page->platform);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
}

coord renderTable_getColWidth(NRenderTable* table, int16 col)
{
    if (col >= 0 && col < table->cols_num)
        return table->cols_width[col];
    else
        return 0;
}
