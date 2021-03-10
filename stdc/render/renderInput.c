/*
 * renderInput.c
 *
 *  Created on: 2011-3-24
 *      Author: wuyulun
 */

#include "renderInc.h"

#define DEFAULT_TEXT_SIZE       20
#define DEFAULT_HORI_SPACING    8
#define DEFAULT_VERT_SPACING    12
#define DEFAULT_SPACING         4
#define DEFAULT_BUTTON_WIDTH    60
#define FIXED_BUTTON_SIZE       14

static uint8 l_label_submit[7] = {0xe6, 0x8f, 0x90, 0xe4, 0xba, 0xa4, 0x0};
static uint8 l_label_reset[7] = {0xe9, 0x87, 0x8d, 0xe7, 0xbd, 0xae, 0x0};
static char l_label_go[3] = {'G', 'O', 0};

#ifdef NBK_MEM_TEST
int renderInput_memUsed(const NRenderInput* ri)
{
    int size = 0;
    if (ri) {
        size = sizeof(NRenderInput);
        size += editBox_memUsed(ri->editor);
        if (ri->text)
            size += nbk_strlen(ri->text) + 1;
        if (ri->textU)
            size += (nbk_wcslen(ri->textU) + 1) * 2;
    }
    return size;
}
#endif

NRenderInput* renderInput_create(void* node)
{
    NRenderInput* r;
#ifdef NBK_USE_MEMMGR
	r = (NRenderInput*)render_alloc();
#else
	r = (NRenderInput*)NBK_malloc0(sizeof(NRenderInput));
#endif
    N_ASSERT(r);
    
    renderNode_init(&r->d);
    
    r->d.type = RNT_INPUT;
    r->d.node = node;
    r->iid = -1;
    
    r->d.Layout = renderInput_layout;
    r->d.Paint = renderInput_paint;
    
    r->type = NEINPUT_TEXT;
    {
        char* p = attr_getValueStr(((NNode*)node)->atts, ATTID_TYPE);
        if (p) {
            if (nbk_strcmp(p, "password") == 0)
                r->type = NEINPUT_PASSWORD;
            else if (nbk_strcmp(p, "submit") == 0)
                r->type = NEINPUT_SUBMIT;
            else if (nbk_strcmp(p, "reset") == 0)
                r->type = NEINPUT_RESET;
            else if (nbk_strcmp(p, "button") == 0)
                r->type = NEINPUT_BUTTON;
            else if (nbk_strcmp(p, "image") == 0)
                r->type = NEINPUT_IMAGE;
            else if (nbk_strcmp(p, "hidden") == 0)
                r->type = NEINPUT_HIDDEN;
            else if (nbk_strcmp(p, "checkbox") == 0)
                r->type = NEINPUT_CHECKBOX;
            else if (nbk_strcmp(p, "radio") == 0)
                r->type = NEINPUT_RADIO;
            else if (nbk_strcmp(p, "file") == 0)
                r->type = NEINPUT_FILE;
            else if (nbk_strcmp(p, "go") == 0)
                r->type = NEINPUT_GO;
        }
    }

    switch (r->type) {
    case NEINPUT_TEXT:
    case NEINPUT_PASSWORD:
    {
        NNode* n = (NNode*)node;
        int16 maxlen = attr_getCoord(n->atts, ATTID_MAXLENGTH);
        char* value = attr_getValueStr(n->atts, ATTID_VALUE);
        char* alt = attr_getValueStr(n->atts, ATTID_ALT);
        r->editor = editBox_create((r->type == NEINPUT_PASSWORD) ? 1 : 0);
        if (maxlen != -1)
            editBox_setMaxLength(r->editor, maxlen);
        if (value)
            editBox_setText(r->editor, (uint8*)value, -1);
        if (alt)
            r->altU = uni_utf8_to_utf16_str((uint8*)alt, nbk_strlen(alt), N_NULL);
        break;
    }
    case NEINPUT_CHECKBOX:
    case NEINPUT_RADIO:
        r->checked = attr_getValueBool(((NNode*)node)->atts, ATTID_CHECKED);
        break;
    }
    
    return r;
}

void renderInput_delete(NRenderInput** ri)
{
    NRenderInput* r = *ri;
    
    if (r->editor)
        editBox_delete(&r->editor);
    if (r->text)
        NBK_free(r->text);
    if (r->textU)
        NBK_free(r->textU);
    if (r->altU)
        NBK_free(r->altU);
    
#ifdef NBK_USE_MEMMGR
	render_free(r);
#else
    NBK_free(r);
#endif
    *ri = N_NULL;
}

static coord get_label(NRenderInput* ri, NPage* page, nbool hint)
{
    NNode* node = (NNode*)ri->d.node;
    char* txt;
    coord w = 0;

    txt = attr_getValueStr(node->atts, ATTID_VALUE);
    //if (txt == N_NULL)
    //    txt = attr_getValueStr(node->atts, ATTID_ALT);

    if (txt == N_NULL && hint) {
        if (ri->type == NEINPUT_SUBMIT || ri->type == NEINPUT_IMAGE)
            txt = (char*)l_label_submit;
        else if (ri->type == NEINPUT_RESET)
            txt = (char*)l_label_reset;
    }
    if (txt == N_NULL && ri->type == NEINPUT_GO) // NBK append
        txt = l_label_go;

    if (txt) {
        char* txtEsc = (char*)NBK_malloc(nbk_strlen(txt) + 1);
        uint8* p = (uint8*)txtEsc;
        int wcLen;
        nbk_unescape(txtEsc, txt);
        while (*p) {
            if (*p < 0x20) *p = 0x20;
            p++;
        }
        if (ri->textU)
            NBK_free(ri->textU);
        ri->textU = uni_utf8_to_utf16_str((uint8*)txtEsc, -1, &wcLen);
        NBK_free(txtEsc);
        w = NBK_gdi_getTextWidth(page->platform, ri->fontId, ri->textU, wcLen);
    }

    return w;
}

void renderInput_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force)
{
    NView* view = (NView*)style->view;
    NPage* page = (NPage*)view->page;
    NRenderInput* ri = (NRenderInput*)rn;
    NNode* node = (NNode*)rn->node;
    coord maxw = renderNode_getContainerWidth(rn);
    coord w, h;
    int16 sz;
    NAvailArea* aa;
    
    if (!rn->needLayout && !force) {
        if (ri->type != NEINPUT_HIDDEN)
            layoutStat_rebuildForInline(stat, rn);
        return;
    }
    
    renderNode_calcStyle(rn, style);

    if (ri->type == NEINPUT_HIDDEN)
        rn->display = CSS_DISPLAY_NONE;
    ri->fontId = NBK_gdi_getFont(page->platform, style->font_size, style->bold, style->italic);
    
    h = NBK_gdi_getFontHeight(page->platform, ri->fontId);
    h += DEFAULT_VERT_SPACING;
    
    w = 0;
    sz = attr_getCoord(node->atts, ATTID_SIZE);
    if (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD) {
        if (sz == -1)
            sz = DEFAULT_TEXT_SIZE;
        sz += 4;
        w = NBK_gdi_getCharWidth(page->platform, ri->fontId, 0x3000) / 2 * sz;
    }
    else if (ri->type == NEINPUT_CHECKBOX || ri->type == NEINPUT_RADIO) {
        w = h = FIXED_BUTTON_SIZE;
    }
    else if (ri->type == NEINPUT_FILE) {
        w = maxw;
    }
    else if (ri->type != NEINPUT_HIDDEN && ri->type != NEINPUT_UNKNOWN) {
        if (sz == -1) {
            ri->picGot = 0;
            
            if (ri->type == NEINPUT_IMAGE && ri->iid != -1 && !ri->picFail) {
                NSize size;
                nbool fail;
                ri->picGot = imagePlayer_getSize(view->imgPlayer, ri->iid, &size, &fail);
                if (ri->picGot) {
                    w = size.w;
                    h = size.h;
                    ri->picFail = N_FALSE;
                }
                else if (fail)
                    ri->picFail = N_TRUE;
            }
            
            if (!ri->picGot) {
                w = get_label(ri, page, N_TRUE);
                if (w == 0)
                    w = DEFAULT_BUTTON_WIDTH;
            }
        }
        else {
            w = sz;
        }
    }
    
    if (   ri->type != NEINPUT_HIDDEN
        && ri->type != NEINPUT_CHECKBOX
        && ri->type != NEINPUT_RADIO
        && ri->type != NEINPUT_FILE
        && ri->type != NEINPUT_UNKNOWN ) {
        
        if (ri->type == NEINPUT_IMAGE && ri->picGot && !ri->picFail)
            ;
        else
            w += DEFAULT_HORI_SPACING;
    }
    
    ri->cssWH = 0;
    if (style->width.type) {
        w = css_calcValue(style->width, maxw, style, w);
        ri->cssWH = 1;
    }
    if (style->height.type && ri->type != NEINPUT_TEXT) {
        h = css_calcValue(style->height, h, style, h);
        ri->cssWH = 1;
    }

    w += rn->margin.l + rn->margin.r;
    h += rn->margin.t + rn->margin.b;
    
    if (w > maxw) w = maxw;

    if (ri->type != NEINPUT_HIDDEN) {
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
    }
        
    rn->needLayout = 0;
    rn->parent->needLayout = 1;
}

// 绘制单行输入控件
static void paint_text(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderInput* ri = (NRenderInput*)rn;
    coord x, y/*, da*/;
    NRect pr, cl;
    NECtrlState st = NECS_NORMAL;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    //da = (ri->cssWH) ? 0 : (coord)float_imul(1, style->zoom);
    //rect_grow(&pr, -da, -da);
    
    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r) + 1;
    cl.b = N_MIN(rect_getHeight(rect), pr.b) + 1;

    if (ri->inEdit)
        st = NECS_INEDIT;
    else if (style->highlight)
        st = NECS_HIGHLIGHT;
    
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (NBK_paintText(page->platform, &pr, st, &cl) == N_FALSE) {
        
        NBK_gdi_setBrushColor(page->platform, &style->ctlFillColor);
        NBK_gdi_fillRect(page->platform, &pr);
        
        if (style->highlight || ri->inEdit) {
            renderNode_drawFocusRing(pr,
                (ri->inEdit) ? style->ctlEditColor : style->ctlFocusColor, page->platform);
        }
        else {
            NBK_gdi_setPenColor(page->platform, &style->ctlFgColor);
            NBK_gdi_drawRect(page->platform, &pr);
        }
    }
    
    NBK_gdi_cancelClippingRect(page->platform);

    if (ri->inEdit) {
        int len;
        wchr* text = renderInput_getEditText16(ri, &len);
        int maxLen = renderInput_getEditMaxLength(ri);
        nbool password = ri->editor->password;
        if (NBK_editInput(page->platform, ri->fontId, &pr, text, len, maxLen, password))
            return;
    }

    if (ri->editor) {
        coord dx = DEFAULT_HORI_SPACING >> 1;
        coord dy = DEFAULT_VERT_SPACING >> 1;
        dx = (coord)float_imul(dx, style->zoom);
        dy = (coord)float_imul(dy, style->zoom);
        rect_grow(&pr, -dx, -dy);
        //rect_grow(&pr, da, da);
        NBK_gdi_setClippingRect(page->platform, &pr);
        pr.r -= 2;
        NBK_gdi_useFont(page->platform, ri->fontId);
        if (!ri->inEdit && ri->editor->use == 0 && ri->altU) {
            NPoint pos;
            pos.x = pr.l;
            pos.y = pr.t;
            NBK_gdi_drawText(page->platform, ri->altU, nbk_wcslen(ri->altU), &pos, 0);
        }
        else
            editBox_draw(ri->editor, ri->fontId, &pr, ri->inEdit, style);
        NBK_gdi_releaseFont(page->platform);
        NBK_gdi_cancelClippingRect(page->platform);
    }
}

// 绘制按钮控件
static void paint_button(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderInput* ri = (NRenderInput*)rn;
    coord x, y, dx, dy/*, da*/;
    NRect pr, cl;
    NPoint pt;
    int len = 0;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    //da = (ri->cssWH) ? 0 : (coord)float_imul(1, style->zoom);
    //rect_grow(&pr, -da, -da);

    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r) + 1;
    cl.b = N_MIN(rect_getHeight(rect), pr.b) + 1;

    if (ri->textU)
        len = nbk_wcslen(ri->textU);
    
    imagePlayer_setDraw(page->view->imgPlayer, rn->bgiid);
    if (rn->bgiid != -1) {
        NRect ir = rn->r;
        rect_move(&ir, ir.l + x, ir.t + y);
        rect_toView(&ir, style->zoom);
        rect_move(&ir, ir.l - rect->l, ir.t - rect->t);
        renderNode_drawBgImage(rn, &ir, rect, style);
    }
    else {
        NBK_gdi_setClippingRect(page->platform, &cl);
        
        if (NBK_paintButton(page->platform, ri->fontId, &pr, ri->textU, len,
                            ((style->highlight) ? NECS_HIGHLIGHT : NECS_NORMAL), &cl)) {
            NBK_gdi_cancelClippingRect(page->platform);
            return;
        }
    }

    if (rn->bgiid == -1) {
        if (rn->hasBgcolor) {
            NBK_gdi_setBrushColor(page->platform, &rn->background_color);
            NBK_gdi_fillRect(page->platform, &pr);
        }
        else { // 绘制渐变色按钮
            renderNode_fillButton(&pr, page->platform);
        }
        
        if (!style->highlight) {
            NBK_gdi_setPenColor(page->platform, &style->ctlFgColor);
            NBK_gdi_drawRect(page->platform, &pr);
        }
        
        NBK_gdi_cancelClippingRect(page->platform);
    }
    
    NBK_gdi_setClippingRect(page->platform, &cl);
    cl = pr;

    if (ri->textU) { // 绘制标签
        coord fw, fh;
        
        fw = NBK_gdi_getTextWidth(page->platform, ri->fontId, ri->textU, len);
        fh = NBK_gdi_getFontHeight(page->platform, ri->fontId);
        
        fw = (coord)float_imul(fw, style->zoom);
        fh = (coord)float_imul(fh, style->zoom);
        
        fw = (fw > rect_getWidth(&pr)) ? rect_getWidth(&pr) : fw;
        dx = (rect_getWidth(&pr) - fw) >> 1;
        dy = (rect_getHeight(&pr) - fh) >> 1;
        
        NBK_gdi_useFont(page->platform, ri->fontId);
        
        pt.x = pr.l + dx;
        pt.y = pr.t + dy;
        NBK_gdi_setPenColor(page->platform, (rn->hasBgcolor) ? &rn->color : &colorBlack);
        NBK_gdi_drawText(page->platform, ri->textU, len, &pt, 0);
        
        NBK_gdi_releaseFont(page->platform);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (style->highlight)
        renderNode_drawFocusRing(pr, style->ctlFocusColor, page->platform);
}

// 绘制图片按钮
static void paint_buttonImage(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NView* view = (NView*)style->view;
    NPage* page = (NPage*)view->page;
    NRenderInput* ri = (NRenderInput*)rn;
    coord x, y;
    NRect pr, cl;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r) + 1;
    cl.b = N_MIN(rect_getHeight(rect), pr.b) + 1;

    NBK_gdi_setClippingRect(page->platform, &cl);
    
    imagePlayer_draw(view->imgPlayer, ri->iid, &pr);
    if (style->highlight)
        renderNode_drawFocusRing(pr, style->ctlFocusColor, page->platform);
    
    NBK_gdi_cancelClippingRect(page->platform);
}

// 绘制复选钮
static void paint_checkbox(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderInput* ri = (NRenderInput*)rn;
    coord x, y, da;
    NRect pr, cl;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;

    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    //da = (coord)float_imul(4, style->zoom);
    //rect_grow(&pr, -da, -da);
    
    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r);
    cl.b = N_MIN(rect_getHeight(rect), pr.b);

    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (NBK_paintCheckbox(page->platform, &pr,
            ((style->highlight) ? NECS_HIGHLIGHT : NECS_NORMAL), ri->checked, &cl)) {
        NBK_gdi_cancelClippingRect(page->platform);
        return;
    }
    
    NBK_gdi_setBrushColor(page->platform, &style->ctlFillColor);
    NBK_gdi_fillRect(page->platform, &pr);
    
    if (!style->highlight) {
        NBK_gdi_setPenColor(page->platform, &style->ctlFgColor);
        NBK_gdi_drawRect(page->platform, &pr);
    }
    
    if (ri->checked) {
        da = (coord)float_imul(2, style->zoom);
        cl = pr;
        rect_grow(&cl, -da, -da);
        NBK_gdi_setBrushColor(page->platform, &style->ctlEditColor);
        NBK_gdi_fillRect(page->platform, &cl);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (style->highlight)
        renderNode_drawFocusRing(pr, style->ctlFocusColor, page->platform);
}

// 绘制单选钮
static void paint_radio(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderInput* ri = (NRenderInput*)rn;
    coord x, y, da;
    NRect pr, cl;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    //da = (coord)float_imul(4, style->zoom);
    //rect_grow(&pr, -da, -da);
    
    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r);
    cl.b = N_MIN(rect_getHeight(rect), pr.b);

    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (NBK_paintRadio(page->platform, &pr,
            ((style->highlight) ? NECS_HIGHLIGHT : NECS_NORMAL), ri->checked, &cl)) {
        NBK_gdi_cancelClippingRect(page->platform);
        return;
    }
    
    if (!style->highlight) {
        NBK_gdi_setPenColor(page->platform, &style->ctlFgColor);
        NBK_gdi_drawEllipse(page->platform, &pr);
    }
    
    if (ri->checked) {
        int i;
        da = (coord)float_imul(2, style->zoom);
        cl = pr;
        rect_grow(&cl, -da, -da);
        NBK_gdi_setPenColor(page->platform, &style->ctlEditColor);
        for (i=0; i < 4; i++) {
            NBK_gdi_drawEllipse(page->platform, &cl);
            rect_grow(&cl, -1, -1);
        }
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (style->highlight)
        renderNode_drawFocusRing(pr, style->ctlFocusColor, page->platform);
}

// 绘制文件选择控件
static void paint_file(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NRenderInput* ri = (NRenderInput*)rn;
    coord x, y/*, da*/;
    NRect pr, cl;
    int len = 0;
    
    renderNode_getAbsPos(rn->parent, &x, &y);

    pr = renderNode_getPaintArea(rn);
    rect_move(&pr, pr.l + x, pr.t + y);
    rect_toView(&pr, style->zoom);
    if (!rect_isIntersect(rect, &pr))
        return;
    
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    //da = (coord)float_imul(1, style->zoom);
    //rect_grow(&pr, -da, -da);

    if (ri->textU == N_NULL) {
        if (ri->text) {
            int wcLen;
            ri->textU = uni_utf8_to_utf16_str((uint8*)ri->text, -1, &wcLen);
            len = wcLen;
        }
    }
    else
        len = nbk_wcslen(ri->textU);
    
    cl.l = cl.t = 0;
    cl.r = N_MIN(rect_getWidth(rect), pr.r) + 1;
    cl.b = N_MIN(rect_getHeight(rect), pr.b) + 1;

    NBK_gdi_setClippingRect(page->platform, &cl);
    
    if (NBK_paintBrowse(page->platform, ri->fontId, &pr, ri->textU, len,
            ((style->highlight) ? NECS_HIGHLIGHT : NECS_NORMAL), &cl)) {
        NBK_gdi_cancelClippingRect(page->platform);
        return;
    }
    
    NBK_gdi_setBrushColor(page->platform, &style->ctlBgColor);
    NBK_gdi_fillRect(page->platform, &pr);
    NBK_gdi_setPenColor(page->platform, &style->ctlFgColor);
    NBK_gdi_drawRect(page->platform, &pr);
    
    if (ri->textU) {
        NPoint pt;
        
        pt.x = pr.l + 2;
        pt.y = pr.t;
        
        NBK_gdi_useFont(page->platform, ri->fontId);
        NBK_gdi_drawText(page->platform, ri->textU, len, &pt, 0);
        NBK_gdi_releaseFont(page->platform);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    if (style->highlight)
        renderNode_drawFocusRing(pr, style->ctlFocusColor, page->platform);
}

void renderInput_paint(NRenderNode* rn, NStyle* style, NRenderRect* rect)
{
    NRenderInput* ri = (NRenderInput*)rn;
    
    switch (ri->type) {
    case NEINPUT_TEXT:
    case NEINPUT_PASSWORD:
        paint_text(rn, style, rect);
        break;
        
    case NEINPUT_SUBMIT:
    case NEINPUT_RESET:
    case NEINPUT_BUTTON:
    case NEINPUT_GO:
        paint_button(rn, style, rect);
        break;
        
    case NEINPUT_IMAGE:
        if (ri->iid == -1 || ri->picFail || !ri->picGot)
            paint_button(rn, style, rect);
        else
            paint_buttonImage(rn, style, rect);
        break;
        
    case NEINPUT_CHECKBOX:
        paint_checkbox(rn, style, rect);
        break;
        
    case NEINPUT_RADIO:
        paint_radio(rn, style, rect);
        break;
        
    case NEINPUT_FILE:
        paint_file(rn, style, rect);
        break;
    }
}

nbool renderInput_isVisible(NRenderInput* ri)
{   
    NRenderNode* r = (NRenderNode*)ri;
    if (r->display == CSS_DISPLAY_NONE)
        return  N_FALSE;
    
    return ((ri->type == NEINPUT_HIDDEN) ? N_FALSE : N_TRUE);
}

nbool renderInput_isEditing(NRenderInput* ri)
{
    return ri->inEdit;
}

void renderInput_edit(NRenderInput* ri, void* doc)
{
    NPage* page = (NPage*)((NDocument*)doc)->page;
    NNode* node = (NNode*)ri->d.node;
    uint8* alt;
    
    if (!renderInput_isVisible(ri))
        return;
    
    alt = (uint8*)attr_getValueStr(node->atts, ATTID_ALT);

    ri->inEdit = !ri->inEdit;
    
    if (ri->inEdit) {
        int len = nbk_strlen((char*)alt);
        int i = 0;
        int8 offset;
        uint8* tooFar = alt + len;
        nbool clearDefault = N_FALSE;
        
        while (alt && alt < tooFar && i < ri->editor->use) {
            wchr tmpWChar = uni_utf8_to_utf16(alt, &offset);
            if (tmpWChar != ri->editor->text[i])
                break;
            alt += offset;
            i++;
        }
        if (alt && alt >= tooFar && i >= ri->editor->use)
            clearDefault = N_TRUE;

        if (clearDefault) {
            editBox_setText16(ri->editor, N_NULL, 0);
        }
        else if (ri->inEdit) {
            ri->editor->anchorPos = 0;
            ri->editor->cur = ri->editor->use;
            NBK_fep_updateCursor(page->platform);
        }
        else if (!ri->inEdit) {
            editBox_clearSelText(ri->editor);
            NBK_fep_updateCursor(page->platform);
        }
        NBK_fep_enable(page->platform);
    }
    else {
        if (ri->editor->use <= 0) {
            if (alt)
                editBox_setText(ri->editor, alt, -1);
        }
        NBK_fep_disable(page->platform);
        editBox_maskAll(ri->editor);
    }
}

nbool renderInput_processKey(NRenderNode* rn, NEvent* event)
{
    NRenderInput* ri = (NRenderInput*)rn;
    NPage* page = (NPage*)event->page;
    
    if (ri->inEdit) {
        nbool handle = N_TRUE;
        NRect vr = rn->r;
        
        if (NEEVENT_PEN == event->type) {
            renderNode_getAbsRect(rn, &vr);
            
            rect_toView(&vr, history_getZoom((NHistory*)page->history));
            if (!rect_hasPt(&vr, event->d.penEvent.pos.x, event->d.penEvent.pos.y))
                handle = N_FALSE;
        }
        
        if (handle)
            return editBox_handleKey(ri->editor, event, &vr, ri->fontId);
    }
    
    return N_FALSE;
}

// need release
char* renderInput_getEditText(NRenderInput* ri)
{
    if ((ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD) && ri->editor)
        return editBox_getText(ri->editor);
    else
        return N_NULL;
}

int renderInput_getEditTextLen(NRenderInput* ri)
{
    if ((ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD) && ri->editor)
        return editBox_getTextLen(ri->editor);
    else
        return 0;
}

int renderInput_getEditLength(NRenderInput* ri)
{
    if (ri->editor)
        return ri->editor->use;
    else
        return 0;
}

int renderInput_getEditMaxLength(NRenderInput* ri)
{
    if (ri->editor)
        return ri->editor->max - 1;
    else
        return 0;
}

void renderInput_getRectOfEditing(NRenderInput* ri, NRect* rect, NFontId* fontId)
{
    NRect r;
    coord x, y;
    
    renderNode_getAbsPos(ri->d.parent, &x, &y);

    r.l = x + ri->d.r.l;
    r.t = y + ri->d.r.t;
    r.r = r.l + rect_getWidth(&ri->d.r);
    r.b = r.t + rect_getHeight(&ri->d.r);
    
    *rect = r;
    *fontId = ri->fontId;
}

void renderInput_setText(NRenderInput* ri, char* text)
{
    if (ri->text)
        NBK_free(ri->text);
    ri->text = text;
    
    if (ri->textU) {
        NBK_free(ri->textU);
        ri->textU = N_NULL;
    }
}

void renderInput_setEditText16(NRenderInput* ri, wchr* text, int len)
{
    if (ri->editor) {
        editBox_setText16(ri->editor, text, len);
    }
}

wchr* renderInput_getEditText16(NRenderInput* ri, int* len)
{
    if (ri->editor) {
        *len = ri->editor->use;
        return ri->editor->text;
    }
    else {
        *len = 0;
        return N_NULL;
    }
}

void renderInput_getSelPos(NRenderInput* ri, int* start, int* end)
{
    if (ri->editor) {
        *start = ri->editor->anchorPos;
        *end = ri->editor->cur;
    }
}

void renderInput_setSelPos(NRenderInput* ri, int start, int end)
{
    nbool left = N_TRUE;
    
    if (ri->editor) {
        ri->editor->anchorPos = start;
        if (ri->editor->cur < end)
            left = N_FALSE;
        
        if (!ri->editor->cur && end == ri->editor->use)
            left = N_TRUE;
        if (!end && ri->editor->cur == ri->editor->use)
            left = N_FALSE;
        
        ri->editor->cur = end;
    }
}

int16 renderInput_getDataSize(NRenderNode* rn, NStyle* style)
{
    NRenderInput* ri = (NRenderInput*)rn;
    NNode* node = (NNode*)rn->node;
    int16 sz, size;
    
    sz = attr_getCoord(node->atts, ATTID_SIZE);
    if (sz == -1) {
        if (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD)
            size = DEFAULT_TEXT_SIZE / 2;
        else if (ri->type == NEINPUT_CHECKBOX || ri->type == NEINPUT_RADIO)
            size = 2;
        else if (ri->type == NEINPUT_HIDDEN)
            size = 0;
        else
            size = DEFAULT_BUTTON_WIDTH / style->font_size;
    }
    else {
        size = sz / 2;
        if (sz % 2)
            size++;
    }
    
    return size;
}
