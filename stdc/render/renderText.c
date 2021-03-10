/*
 * renderText.c
 *
 *  Created on: 2010-12-28
 *      Author: wuyulun
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderText_memUsed(const NRenderText* rt)
{
    int size = 0;
    if (rt) {
        size = sizeof(NRenderText);
        if (rt->lines)
            size += sizeof(NRTLine) * rt->max;
    }
    return size;
}
#endif

static wchr* rt_skip_space_prefix(wchr* text)
{
    wchr* p = text;
    while (*p && *p == 0x20)
        p++;
    return ((*p) ? p : text);
}

#define IS_LETTER(c) ((c >= 97 && c <= 122) || (c >= 65 && c <= 90))
#define IS_NUMBER(c) (c >= 48 && c <= 57)

enum NCharType {
    NECT_NONE,
    NECT_LETTER,
    NECT_NUMBER
};

static wchr* rt_break_line(void* pfd, const wchr* text, NFontId fontId, coord maxw, coord* width, nbool zoom)
{
    coord w, tw = 0, bw;
    wchr* p = (wchr*)text;
    wchr* b = N_NULL;
    nid ct, lct = NECT_NONE;
    
    while (*p) {
        if (IS_LETTER(*p))
            ct = NECT_LETTER;
        else if (IS_NUMBER(*p))
            ct = NECT_NUMBER;
        else
            ct = NECT_NONE;
        
        if (ct) {
            if (b == N_NULL || ct != lct) {
                b = p;
                bw = tw;
                lct = ct;
            }
        }
        else {
            b = N_NULL;
            lct = NECT_NONE;
        }
        
        w = (zoom) ? NBK_gdi_getCharWidthByEditor(pfd, fontId, *p) \
                   : NBK_gdi_getCharWidth(pfd, fontId, *p);
        if (tw + w > maxw)
            break;
        
        tw += w;
        p++;
    }
    
    *width = tw;
    if (*p && b) {
        p = b;
        *width = bw;
    }
    return p;
}

NRenderText* renderText_create(void* node)
{
    NNode* n = (NNode*)node;
    NRenderText* rt;
#ifdef NBK_USE_MEMMGR
	rt = (NRenderText*)render_alloc();
#else
	rt = (NRenderText*)NBK_malloc0(sizeof(NRenderText));
#endif
    N_ASSERT(rt);
    
    renderNode_init(&rt->d);
    
    rt->d.type = RNT_TEXT;
    rt->d.node = node;
    
    rt->d.Layout = renderText_layout;
    rt->d.Paint = renderText_paint;
    
    return rt;
}

void renderText_delete(NRenderText** rt)
{
    NRenderText* t = *rt;
#ifdef NBK_USE_MEMMGR
	render_free(t);
#else
    NBK_free(t);
#endif
    *rt = N_NULL;
}

void renderText_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderText* rt = (NRenderText*)rn;
    NNode* node = (NNode*)rn->node;
    coord tw, mw, lh, fh;
    wchr *p, *q;
    NAvailArea* aa;
    NRenderTextPiece* rtp;
    NRenderNode* r;
    
    if (!rn->needLayout && !force) {
        if (!rn->init) {
            rn->init = 1;
            r = rn->child;
            while (r) {
                layoutStat_rebuildForInline(stat, r);
                r = r->next;
            }
        }
        else {
            rn->init = 0;
        }
        return;
    }

    if (!rn->init) {
        rn->init = 1;

        if (rn->child) {
            r = rn->child;
            while (r) {
                rtp = (NRenderTextPiece*)r;
                r = r->next;
                renderTextPiece_delete(&rtp);
            }
            rn->child = N_NULL;
        }
        
        renderNode_calcStyle(rn, style);
    
        rt->underline = style->underline;
        rt->clip = *style->clip;
    
        rt->fontId = NBK_gdi_getFont(page->platform, style->font_size, style->bold, style->italic);

        fh = NBK_gdi_getFontHeight(page->platform, rt->fontId);
        rt->line_spacing = DEFAULT_LINE_SPACE;
        if (style->line_height.type) {
            lh = css_calcValue(style->line_height, fh, style, fh);
            if (lh > fh)
                rt->line_spacing = lh - fh;
        }
        lh = fh + rt->line_spacing;
    
        rt->indent = css_calcValue(style->text_indent, style->font_size, style, 0);

        if (node->space) {
            r = renderNode_getPrevNode(rn);
            if (r == N_NULL || r->type != RNT_TEXTPIECE) {
                rn->init = 0;
                rn->needLayout = 0;
                return;
            }
        }
    
        r = N_NULL;
        rtp = N_NULL;
        aa = N_NULL;
        rn->r.l = rn->r.t = N_MAX_COORD;
        rn->r.r = rn->r.b = 0;
        p = node->d.text;
        while (*p) {

            if (rtp == N_NULL)
                rtp = renderTextPiece_create();

            if (aa == N_NULL)
                aa = layoutStat_getAA(stat, (NRenderNode*)rtp, lh);
            if (aa->multiline)
                p = rt_skip_space_prefix(p);
        
            mw = rect_getWidth(&aa->r);
            q = rt_break_line(page->platform, p, rt->fontId, mw, &tw, N_FALSE);
        
            if (q > p) {
                rtp->text = p;
                rtp->len = q - p;

                rtp->d.r.l = aa->r.l;
                rtp->d.r.t = aa->r.t;
                rtp->d.r.r = rtp->d.r.l + tw;
                rtp->d.r.b = rtp->d.r.t + lh;
                rtp->d.og_r = rtp->d.r;
                rtp->d.aa_r = aa->r;

                layoutStat_updateAA(stat, (NRenderNode*)rtp, aa);
                aa = N_NULL;

                rn->r.l = N_MIN(rn->r.l, rtp->d.r.l);
                rn->r.t = N_MIN(rn->r.t, rtp->d.r.t);
                rn->r.r = N_MAX(rn->r.r, rtp->d.r.r);
                rn->r.b = N_MAX(rn->r.b, rtp->d.r.b);

                rtp->d.parent = rn;
                if (r == N_NULL) {
                    rn->child = (NRenderNode*)rtp;
                    r = rn->child;
                }
                else {
                    r->next = (NRenderNode*)rtp;
                    rtp->d.prev = r;
                    r = (NRenderNode*)rtp;
                }

                rtp = N_NULL;
                p = q;
            }
            else {
                if (layoutStat_isLastAA(stat, aa)) { // can't places any text
                    rn->init = 0;
                    rn->needLayout = 0;
                    break;
                }
                aa = layoutStat_getNextAA(stat, aa, N_TRUE, lh);
            }
        }

        if (rtp)
            renderTextPiece_delete(&rtp);
    }
    else {
        rn->init = 0;
        rn->needLayout = 0;
        rn->parent->needLayout = 1;
    }
}

void renderText_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderText* rt = (NRenderText*)rn;
    coord x, y, fh, da, py;
    NPoint pos;
    NRect pr, cl;
    NRenderNode* r;
    NRenderTextPiece* rtp;
    
    if (rn->display == CSS_DISPLAY_NONE)
        return;
    
    if (rn->child == N_NULL)
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
    pr = rt->clip;
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    cl.r = N_MIN(cl.r, pr.r);
    cl.b = N_MIN(cl.b, pr.b);
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    fh = NBK_gdi_getFontHeight(page->platform, rt->fontId);
    da = rt->line_spacing >> 1;
    
    fh = (coord)float_imul(fh, style->zoom);
    da = (coord)float_imul(da, style->zoom);
    
    NBK_gdi_useFont(page->platform, rt->fontId);
    
    //rn->hasBgcolor = 1;
    //rn->background_color = colorYellow;
    
    if (style->highlight) {
        NBK_gdi_setPenColor(page->platform, &style->color);
        NBK_gdi_setBrushColor(page->platform, &style->background_color);
    }
    else {
        NBK_gdi_setPenColor(page->platform, &rn->color);
        if (rn->hasBgcolor)
            NBK_gdi_setBrushColor(page->platform, &rn->background_color);
    }
    
    r = rn->child;
    while (r) {
        rtp = (NRenderTextPiece*)r;

        py = (coord)float_imul(r->r.t + y, style->zoom);
        if (py >= rect->b)
            break;

        if (py + da + fh > rect->t) {
            if ((rn->hasBgcolor || style->highlight) && !style->belongA) {
                pr = r->r;
                rect_move(&pr, pr.l + x, pr.t + y);
                rect_toView(&pr, style->zoom);
                rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
                NBK_gdi_fillRect(page->platform, &pr);
            }

            pos.x = (coord)float_imul(r->r.l + x, style->zoom) - rect->l;
            pos.y = py + da - rect->t;
        
            NBK_gdi_drawText(page->platform, rtp->text, rtp->len, &pos, rt->underline);
        }
        r = r->next;
    }
    
    NBK_gdi_releaseFont(page->platform);

    NBK_gdi_cancelClippingRect(page->platform);
}

nbool renderText_hasPt(NRenderText* rt, NPoint pt)
{
    coord x, y;
    NRenderNode* r;
    NRect rect;
    
    renderNode_getAbsPos(rt->d.parent, &x, &y);

    r = rt->d.child;
    while (r) {
        rect = r->r;
        rect_move(&rect, x + rect.l, y + rect.t);
        if (rect_hasPt(&rect, pt.x, pt.y))
            return N_TRUE;
        r = r->next;
    }
    
    return N_FALSE;
}

int16 renderText_getDataSize(NRenderNode* rn, NStyle* style)
{
    NNode* node = (NNode*)rn->node;
    return node->len;
}
