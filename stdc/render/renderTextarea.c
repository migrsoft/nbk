/*
 * renderTextarea.c
 *
 *  Created on: 2011-4-9
 *      Author: migr
 */

#include "renderInc.h"

#define DEFAULT_HORI_SPACING    4
#define DEFAULT_VERT_SPACING    4
#define DEFAULT_SPACING         4
#define DEFAULT_COLS            40
#define DEFAULT_ROWS            3

#ifdef NBK_MEM_TEST
int renderTextarea_memUsed(const NRenderTextarea* rt)
{
    int size = 0;
    if (rt) {
        size = sizeof(NRenderTextarea);
        size += textEditor_memUsed(rt->editor);
    }
    return size;
}
#endif

NRenderTextarea* renderTextarea_create(void* node)
{
    NRenderTextarea* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderTextarea*)render_alloc();
#else
	r = (NRenderTextarea*)NBK_malloc0(sizeof(NRenderTextarea));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_TEXTAREA;
    r->d.node = node;
    
    r->d.Layout = renderTextarea_layout;
    r->d.Paint = renderTextarea_paint;
    
    r->editor = textEditor_create();
    
    { // alt used in a non-standard way
        NNode* n = (NNode*)node;
        char* alt = attr_getValueStr(n->atts, ATTID_ALT);
        if (alt)
            r->altU = uni_utf8_to_utf16_str((uint8*)alt, nbk_strlen(alt), N_NULL);
    }
    
    return r;
}

void renderTextarea_delete(NRenderTextarea** area)
{
    NRenderTextarea* r = *area;
    textEditor_delete(&r->editor);
    if (r->altU)
        NBK_free(r->altU);
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *area = N_NULL;
}

void renderTextarea_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderTextarea* rt = (NRenderTextarea*)rn;
    NNode* node = (NNode*)rn->node;
    coord maxw = renderNode_getContainerWidth(rn);
    coord w, h;
    int /*cols,*/ rows;
    NAvailArea* aa;
    
    if (!rn->needLayout && !force) {
        layoutStat_rebuildForInline(stat, rn);
        return;
    }
    
    renderNode_calcStyle(rn, style);

    rt->pfd = page->platform;
    
    rt->fontId = NBK_gdi_getFont(page->platform, style->font_size, style->bold, style->italic);
    
    //cols = attr_getCoord(node->atts, ATTID_COLS);
    //if (cols == -1)
    //    cols = DEFAULT_COLS;
    rows = attr_getCoord(node->atts, ATTID_ROWS);
    if (rows == -1)
        rows = DEFAULT_ROWS;
    else if (rows > 5)
        rows = 5;
    
    w = maxw - DEFAULT_HORI_SPACING;
    
    h = (NBK_gdi_getFontHeight(page->platform, rt->fontId) + 2) * rows;
    h += DEFAULT_VERT_SPACING;
    h += rn->margin.t + rn->margin.b;

    aa = layoutStat_getAA(stat, rn, h);
    while (rect_getWidth(&aa->r) < w)
        aa = layoutStat_getNextAA(stat, aa, N_TRUE, h);

    rn->r.l = aa->r.l;
    rn->r.t = aa->r.t;
    rn->r.r = rn->r.l + w;
    rn->r.b = rn->r.t + h;
    rn->og_r = rn->r;
    rn->aa_r = aa->r;

    layoutStat_updateAA(stat, rn, aa);
    
    rt->editor->fontId = rt->fontId;
    rt->editor->vis = rows;
    rt->editor->width = rect_getWidth(&rn->r) - DEFAULT_HORI_SPACING - 6;
    
    if (node->child && node->child->id == TAGID_TEXT)
        textEditor_setText(rt->editor, node->child->d.text, node->child->len, page->platform);

    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

void renderTextarea_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderTextarea* rt = (NRenderTextarea*)rn;
    coord x, y, dx, dy;
    NRect pr, cl;
    NECtrlState st = NECS_NORMAL;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    //rect_grow(&pr, -1, -1);
    
    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r) + 1;
    cl.b = N_MIN(rect_getHeight(rect), pr.b) + 1;
    
    if (rt->inEdit)
        st = NECS_INEDIT;
    else if (style->highlight)
        st = NECS_HIGHLIGHT;
    
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (NBK_paintTextarea(page->platform, &pr, st, &cl) == N_FALSE) {
        NBK_gdi_setBrushColor(page->platform, &style->ctlFillColor);
        NBK_gdi_fillRect(page->platform, &pr);
        
        if (style->highlight) {
            renderNode_drawFocusRing(pr,
                (rt->inEdit) ? style->ctlEditColor : style->ctlFocusColor, page->platform);
        }
        else {
            NBK_gdi_setPenColor(page->platform, &style->ctlFgColor);
            NBK_gdi_drawRect(page->platform, &pr);
        }
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (rt->inEdit) {
        int len;
        wchr* text = renderTextarea_getText16(rt, &len);
        int maxLen = renderTextarea_getEditMaxLength(rt);
        if (NBK_editTextarea(page->platform, rt->fontId, &pr, text, len, maxLen))
            return;
    }

    dx = DEFAULT_HORI_SPACING >> 1;
    dy = DEFAULT_VERT_SPACING >> 1;
    dx = (coord)float_imul(dx, style->zoom);
    dy = (coord)float_imul(dy, style->zoom);
    rect_grow(&pr, -dx, -dy);
    NBK_gdi_setClippingRect(page->platform, &pr);
    if (!rt->inEdit && rt->editor->use == 0 && rt->altU) {
        NPoint pos;
        pos.x = pr.l;
        pos.y = pr.t;
        NBK_gdi_useFont(page->platform, rt->fontId);
        NBK_gdi_drawText(page->platform, rt->altU, nbk_wcslen(rt->altU), &pos, 0);
        NBK_gdi_releaseFont(page->platform);
    }
    else
        textEditor_draw(rt->editor, &pr, rt->inEdit, style);
    NBK_gdi_cancelClippingRect(page->platform);
}

int renderTextarea_getEditTextLen(NRenderTextarea* rt)
{
    return textEditor_getTextLen(rt->editor);
}

// need release
char* renderTextarea_getEditText(NRenderTextarea* rt)
{
    return textEditor_getText(rt->editor);
}

void renderTextarea_edit(NRenderTextarea* rt, void* doc)
{
    NPage* page = (NPage*)((NDocument*)doc)->page;
    NNode* node = (NNode*) rt->d.node;
    uint8* defaulTxt = N_NULL;
    nbool lastInEdit = N_FALSE;    
    defaulTxt = (uint8*) attr_getValueStr(node->atts, ATTID_ALT);
    lastInEdit = rt->inEdit;
    
    rt->inEdit = !rt->inEdit;
    
    if (rt->inEdit) {
        nbool clearDefault = N_FALSE;
        int len = nbk_strlen((char*) defaulTxt);
        int i = 0;
        int8 offset;
        uint8* tooFar = defaulTxt + len;
        
        NBK_fep_enable(page->platform);
       
        while (defaulTxt && defaulTxt < tooFar && i < rt->editor->use) {
            wchr tmpWChar = uni_utf8_to_utf16((uint8*) defaulTxt, &offset);
            if (tmpWChar != rt->editor->text[i]) {
                break;
            }
            defaulTxt += offset;
            i++;
        }
        if (defaulTxt && defaulTxt >= tooFar && i >= rt->editor->use)
            clearDefault = N_TRUE;

        if (clearDefault) {
            textEditor_setText(rt->editor, N_NULL, 0, page->platform);
        }
        else if (!lastInEdit) {
            textEditor_SeleAllText(rt->editor,page->platform);
            NBK_fep_updateCursor(page->platform);
        }
        else if (lastInEdit) {
            textEditor_clearSelText(rt->editor);
            NBK_fep_updateCursor(page->platform);
        }
    }
    else {
        if (rt->editor->use <= 0) {
            if (defaulTxt)
            {
                wchr* tmp = uni_utf8_to_utf16_str((uint8*) defaulTxt, nbk_strlen((char*)defaulTxt), N_NULL);
                textEditor_setText(rt->editor, tmp, nbk_wcslen(tmp), page->platform);
                NBK_free(tmp);
            }
        }
        
        NBK_fep_disable(page->platform);
    }
}

nbool renderTextarea_processKey(NRenderNode* rn, NEvent* event)
{
    NRenderTextarea* rt = (NRenderTextarea*)rn;
    NPage* page = (NPage*)event->page;

    if (rt->inEdit) {
        nbool handle = N_TRUE;
        NRect vr = rn->r;

        if (NEEVENT_PEN == event->type) {
            renderNode_getAbsRect(rn, &vr);
            
            rect_toView(&vr, history_getZoom((NHistory*)page->history));
            if (!rect_hasPt(&vr, event->d.penEvent.pos.x, event->d.penEvent.pos.y))
                handle = N_FALSE;
        }

        if (handle)
            return textEditor_handleKey(rt->editor, event, &vr, rt->fontId);
    }

    return N_FALSE;
}

int renderTextarea_getEditLength(NRenderTextarea* rt)
{
    if (rt->editor)
        return rt->editor->use;
    else
        return 0;
}

int renderTextarea_getEditMaxLength(NRenderTextarea* rt)
{
    if (rt->editor)
        return rt->editor->max;
    else
        return 0;
}

void renderTextarea_getRectOfEditing(NRenderTextarea* rt, NRect* rect, NFontId* fontId)
{
    NRect r;
    coord x, y;
    
    renderNode_getAbsPos(rt->d.parent, &x, &y);

    r.l = x + rt->d.r.l;
    r.r = r.l + rect_getWidth(&rt->d.r);

    r.t = y + rt->d.r.t;
    r.t += textEditor_getYOfEditing(rt->editor, rt->pfd);
    r.b = r.t + NBK_gdi_getFontHeight(rt->pfd, rt->fontId);
    
    *rect = r;
    *fontId = rt->fontId;
}

nbool renderTextarea_isEditing(NRenderTextarea* rt)
{
    return rt->inEdit;
}

wchr* renderTextarea_getText16(NRenderTextarea* rt, int* len)
{
    if (rt->editor) {
        *len = rt->editor->use;
        return rt->editor->text;
    }
    else {
        *len = 0;
        return N_NULL;
    }
}

void renderTextarea_setText16(NRenderTextarea* rt, wchr* text, int len)
{
    if (rt->editor) {
        textEditor_setText(rt->editor, text, len, rt->pfd);
    }
}

void renderTextarea_getSelPos(NRenderTextarea* rt, int* start, int* end)
{
    if (rt->editor) {
        *start = rt->editor->anchorPos;
        *end = rt->editor->cur;
    }
}

void renderTextarea_setSelPos(NRenderTextarea* rt, int start, int end)
{
    if (rt->editor) {
        rt->editor->anchorPos = start;
        rt->editor->cur = end;
    }
}

int16 renderTextarea_getDataSize(NRenderNode* rn, NStyle* style)
{
    NNode* node = (NNode*)rn->node;
    int16 cols, size;
    
    cols = attr_getCoord(node->atts, ATTID_COLS);
    if (cols == -1)
        cols = DEFAULT_COLS;
    
    size = cols / 2;
    if (cols % 2)
        size++;
    
    return size;
}
