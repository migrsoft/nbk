/*
 * renderNode.c
 *
 *  Created on: 2010-12-28
 *      Author: wuyulun
 */

#include "renderInc.h"

#ifdef NBK_MEM_TEST
int renderNode_memUsed(const NRenderNode* rn)
{
    int size = 0;
    if (rn) {
        switch (rn->type) {
        case RNT_BLANK:
            size = renderBlank_memUsed((NRenderBlank*)rn);
            break;
        case RNT_BLOCK:
            size = renderBlock_memUsed((NRenderBlock*)rn);
            break;
        case RNT_TEXT:
            size = renderText_memUsed((NRenderText*)rn);
            break;
        case RNT_IMAGE:
            size = renderImage_memUsed((NRenderImage*)rn);
            break;
        case RNT_INPUT:
            size = renderInput_memUsed((NRenderInput*)rn);
            break;
        case RNT_BR:
            size = renderBr_memUsed((NRenderBr*)rn);
            break;
        case RNT_HR:
            size = renderHr_memUsed((NRenderHr*)rn);
            break;
        case RNT_SELECT:
            size = renderSelect_memUsed((NRenderSelect*)rn);
            break;
        case RNT_TEXTAREA:
            size = renderTextarea_memUsed((NRenderTextarea*)rn);
            break;
        case RNT_OBJECT:
            size = renderObject_memUsed((NRenderObject*)rn);
            break;
        }
    }
    return size;
}
#endif

nbool renderNode_has(NNode* node, nid type)
{
    nid id = node->id;
    if (id > TAGID_CLOSETAG)
        id -= TAGID_CLOSETAG;
    
    switch (id) {
    case TAGID_BODY:
    case TAGID_CARD:
    case TAGID_DIV:
    case TAGID_P:
    case TAGID_UL:
    case TAGID_OL:
    case TAGID_LI:
    case TAGID_IMG:
    case TAGID_TEXT:
    case TAGID_BR:
    case TAGID_BUTTON:
    case TAGID_INPUT:
    case TAGID_TEXTAREA:
    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
    case TAGID_HR:
    case TAGID_SELECT:
    case TAGID_OBJECT:
    case TAGID_TABLE:
    case TAGID_TR:
    case TAGID_TD:
    case TAGID_FORM:
    case TAGID_A:
    case TAGID_SPAN:
    case TAGID_STRONG:
        return N_TRUE;
    // HTML5
    case TAGID_HEADER:
    case TAGID_SECTION:
    case TAGID_ARTICLE:
    case TAGID_ASIDE:
    case TAGID_FOOTER:
    case TAGID_NAV:
        return N_TRUE;
    default:
        return N_FALSE;
    }
}

NRenderNode* renderNode_create(void* view, NNode* node)
{
    NView* v = (NView*)view;
    nid id = node->id;
    
    switch (id) {
    case TAGID_BODY:
    case TAGID_CARD:
    case TAGID_DIV:
    case TAGID_P:
    case TAGID_UL:
    case TAGID_OL:
    case TAGID_LI:
    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
    case TAGID_FORM:
    case TAGID_HEADER:
    case TAGID_SECTION:
    case TAGID_ARTICLE:
    case TAGID_ASIDE:
    case TAGID_FOOTER:
    case TAGID_NAV:
    {
        NRenderBlock* r = renderBlock_create(node);
        return (NRenderNode*)r;
    }
    case TAGID_IMG:
    {
        NRenderImage* r = renderImage_create(node);
        char* src = attr_getValueStr(node->atts, ATTID_SRC);
        if (src) {
            if (nbk_strncmp(src, "res://", 6) == 0)
                r->iid = IMAGE_STOCK_TYPE_ID;
            else
                r->iid = imagePlayer_getId(v->imgPlayer, src, NEIT_IMAGE);
        }
        return (NRenderNode*)r;
    }
    case TAGID_TEXT:
    {
        NRenderText* r = renderText_create(node);
        return (NRenderNode*)r;
    }
    case TAGID_HR:
    {
        return (NRenderNode*)renderHr_create(node);
    }
    case TAGID_BR:
    {
        NRenderBr* r = renderBr_create(node);
        return (NRenderNode*)r;
    }
    case TAGID_TEXTAREA:
    {
        return (NRenderNode*)renderTextarea_create(node);
    }
    case TAGID_BUTTON:
    case TAGID_INPUT:
    {
        NRenderInput* r = renderInput_create(node);
        char* src = attr_getValueStr(node->atts, ATTID_SRC);
        if (src)
            r->iid = imagePlayer_getId(v->imgPlayer, src, NEIT_IMAGE);
        return (NRenderNode*)r;
    }
    case TAGID_SELECT:
    {
        return (NRenderNode*)renderSelect_create(node);
    }
	case TAGID_OBJECT:
	{
		return (NRenderNode*)renderBlock_create(node);
	}
    case TAGID_A:
    {
		return (NRenderNode*)renderInline_create(node);
    }
    case TAGID_TABLE:
    {
        return (NRenderNode*)renderTable_create(node);
    }
    case TAGID_TR:
    {
        return (NRenderNode*)renderTr_create(node);
    }
    case TAGID_TD:
    {
        return (NRenderNode*)renderTd_create(node);
    }
    case TAGID_SPAN:
    case TAGID_STRONG:
    {
		return (NRenderNode*)renderInline_create(node);
    }
    default:
        return N_NULL;
    }
}

void renderNode_delete(NRenderNode** rn)
{
    NRenderNode* t = *rn;
    
    switch (t->type) {
    case RNT_BLOCK:
    {
        NRenderBlock* r = (NRenderBlock*)t;
        renderBlock_delete(&r);
        break;
    }
    case RNT_INLINE:
    {
        NRenderInline* r = (NRenderInline*)t;
        renderInline_delete(&r);
        break;
    }
    case RNT_INLINE_BLOCK:
    {
        NRenderInlineBlock* r = (NRenderInlineBlock*)t;
        renderInlineBlock_delete(&r);
        break;
    }
    case RNT_IMAGE:
    {
        NRenderImage* r = (NRenderImage*)t;
        renderImage_delete(&r);
        break;
    }
    case RNT_TEXT:
    {
        NRenderText* r = (NRenderText*)t;
        renderText_delete(&r);
        break;
    }
    case RNT_TEXTPIECE:
    {
        NRenderTextPiece* r = (NRenderTextPiece*)t;
        renderTextPiece_delete(&r);
        break;
    }
    case RNT_HR:
    {
        NRenderHr* r = (NRenderHr*)t;
        renderHr_delete(&r);
        break;
    }
    case RNT_BR:
    {
        NRenderBr* r = (NRenderBr*)t;
        renderBr_delete(&r);
        break;
    }
    case RNT_INPUT:
    {
        NRenderInput* r = (NRenderInput*)t;
        renderInput_delete(&r);
        break;
    }
    case RNT_SELECT:
    {
        NRenderSelect* r = (NRenderSelect*)t;
        renderSelect_delete(&r);
        break;
    }
    case RNT_TEXTAREA:
    {
        NRenderTextarea* r = (NRenderTextarea*)t;
        renderTextarea_delete(&r);
        break;
    }
    case RNT_OBJECT:
    {
        NRenderObject* r = (NRenderObject*)t;
        renderObject_delete(&r);
        break;
    }
    case RNT_BLANK:
    {
        NRenderBlank* r = (NRenderBlank*)t;
        renderBlank_delete(&r);
        break;
    }
    case RNT_A:
    {
        NRenderA* r = (NRenderA*)t;
        renderA_delete(&r);
        break;
    }
    case RNT_TABLE:
    {
        NRenderTable* r = (NRenderTable*)t;
        renderTable_delete(&r);
        break;
    }
    case RNT_TR:
    {
        NRenderTr* r = (NRenderTr*)t;
        renderTr_delete(&r);
        break;
    }
    case RNT_TD:
    {
        NRenderTd* r = (NRenderTd*)t;
        renderTd_delete(&r);
        break;
    }
    default:
        N_KILL_ME();
        break;
    }
}

void renderNode_init(NRenderNode* rn)
{
    rn->type = RNT_NONE;
    rn->init = rn->force = 0;
    rn->needLayout = 1;
    rn->zindex = 0; // basic panel
    rn->display = CSS_DISPLAY_BLOCK;
    rn->flo = CSS_FLOAT_NONE;
    rn->bgiid = -1;
}

void renderNode_calcStyle(NRenderNode* rn, NStyle* style)
{
    rn->flo = style->flo;
    rn->clr = style->clr;
    rn->display = style->display;
    rn->vert_align = style->vert_align;
    
    rn->margin = style->margin;
    rn->border = style->border;
    rn->padding = style->padding;

    // foreground or text color
    rn->color = style->color;
    
    // background color
    if (style->hasBgcolor) {
        rn->hasBgcolor = 1;
        rn->background_color = style->background_color;
		if (style->opacity > 0.0f)
			rn->background_color.a = (uint8)(255 * style->opacity);
    }
    
    // border color
    if (style->hasBrcolorL) {
        rn->hasBrcolorL = 1;
        rn->border_color_l = style->border_color_l;
    }
    if (style->hasBrcolorT) {
        rn->hasBrcolorT = 1;
        rn->border_color_t = style->border_color_t;
    }
    if (style->hasBrcolorR) {
        rn->hasBrcolorR = 1;
        rn->border_color_r = style->border_color_r;
    }
    if (style->hasBrcolorB) {
        rn->hasBrcolorB = 1;
        rn->border_color_b = style->border_color_b;
    }
    
    // background image
    if (style->bgImage) {
        rn->bg_repeat = style->bg_repeat;
        rn->bg_x_t = style->bg_x_t; rn->bg_x = style->bg_x;
        rn->bg_y_t = style->bg_y_t; rn->bg_y = style->bg_y;
    }
}

nbool renderNode_layoutable(NRenderNode* rn, coord maxw)
{
    if (maxw <= 0) {
        rn->needLayout = 0;
        rn->display = CSS_DISPLAY_NONE;
        return N_FALSE;
    }
    else
        return N_TRUE;
}

nbool renderNode_isChildsNeedLayout(NRenderNode* rn)
{
    NRenderNode* r = rn;
    while (r) {
        if (r->needLayout)
            return N_TRUE;
        r = r->next;
    }
    return N_FALSE;
}

void renderNode_adjustPos(NRenderNode* rn, coord dy)
{
    rn->r.t += dy;
    rn->r.b += dy;
    
    if (rn->type == RNT_IMAGE) {
        NRenderImage* r = (NRenderImage*)rn;
        r->clip.t += dy;
        r->clip.b += dy;
    }
    if (rn->type == RNT_TEXT) {
        NRenderText* r = (NRenderText*)rn;
        r->clip.t += dy;
        r->clip.b += dy;
    }
}

void renderNode_absRect(NRenderNode* rn, NRect* rect)
{
    if (rn->parent) {
        renderNode_getAbsPos(rn->parent, &rect->l, &rect->t);
    }
    else {
        rect->l = rect->t = 0;
    }
    rect->l += rn->r.l;
    rect->r = rect->l + rect_getWidth(&rn->r);
    rect->t += rn->r.t;
    rect->b = rect->t + rect_getHeight(&rn->r);
}

void style_init(NStyle* style)
{
    NColor editColor = {0xff, 0x8c, 0x0, 0xff};
    NColor focusColor = {0x03, 0x5c, 0xfe, 0xff};
    
    NBK_memset(style, 0, sizeof(NStyle));
    
    style->ctlTextColor = colorBlack;
    style->ctlFgColor = colorGray;
    style->ctlBgColor = colorLightGray;
    style->ctlFillColor = colorWhite;
    style->ctlFocusColor = focusColor;
    style->ctlEditColor = editColor;
    
    style->opacity = 1.0f;
    
    style->drawPic = style->drawBgimg = 1;
}

int renderNode_getEditLength(NRenderNode* rn)
{
    if (rn == N_NULL)
        return 0;
    
    if (rn->type == RNT_INPUT)
        return renderInput_getEditLength((NRenderInput*)rn);
    else if (rn->type == RNT_TEXTAREA)
        return renderTextarea_getEditLength((NRenderTextarea*)rn);
    else
        return 0;
}

int renderNode_getEditMaxLength(NRenderNode* rn)
{
    if (rn == N_NULL)
        return 0;
    
    if (rn->type == RNT_INPUT)
        return renderInput_getEditMaxLength((NRenderInput*)rn);
    else if (rn->type == RNT_TEXTAREA)
        return renderTextarea_getEditMaxLength((NRenderTextarea*)rn);
    else
        return 0;
}

nbool renderNode_getRectOfEditing(NRenderNode* rn, NRect* rect, NFontId* fontId)
{
    if (rn == N_NULL)
        return N_FALSE;
    
    if (rn->type == RNT_INPUT) {
        renderInput_getRectOfEditing((NRenderInput*)rn, rect, fontId);
        return N_TRUE;
    }
    else if (rn->type == RNT_TEXTAREA) {
        renderTextarea_getRectOfEditing((NRenderTextarea*)rn, rect, fontId);
        return N_TRUE;
    }
    else
        return N_FALSE;
}

wchr* renderNode_getEditText16(NRenderNode* rn, int* len)
{
    *len = 0;

    if (rn) {
        if (rn->type == RNT_INPUT)
            return renderInput_getEditText16((NRenderInput*)rn, len);
        else if (rn->type == RNT_TEXTAREA)
            return renderTextarea_getText16((NRenderTextarea*)rn, len);
    }
    
    return N_NULL;
}

void renderNode_setEditText16(NRenderNode* rn, wchr* text, int len)
{
    if (rn) {
        int l = (len == -1) ? nbk_wcslen(text) : len;
        if (rn->type == RNT_INPUT)
            renderInput_setEditText16((NRenderInput*)rn, text, l);
        else if (rn->type == RNT_TEXTAREA)
            renderTextarea_setText16((NRenderTextarea*)rn, text, l);
    }
}

nbool renderNode_getAbsRect(NRenderNode* rn, NRect* rect)
{
    if (rn->parent == N_NULL)
        return N_FALSE;
    
    switch (rn->type) {
    case RNT_BLOCK:
    case RNT_TEXT:
    case RNT_TEXTPIECE:
    case RNT_IMAGE:
    case RNT_INPUT:
    case RNT_TEXTAREA:
    case RNT_SELECT:
    {
        NRenderNode* parent = (rn->type == RNT_TEXTPIECE) ? rn->parent->parent : rn->parent;
        coord x, y;
        renderNode_getAbsPos(parent, &x, &y);
        rect->l = x + rn->r.l;
        rect->t = y + rn->r.t;
        rect->r = rect->l + rect_getWidth(&rn->r);
        rect->b = rect->t + rect_getHeight(&rn->r);
        return N_TRUE;
    }
    }

    return N_FALSE;
}

void renderNode_drawBorder(NRenderNode* rn, NCssBox* border, NRect* rect, void* pfd)
{
    NRect l;
    
    if (rn->hasBrcolorL && border->l) {
        l = *rect;
        l.r = l.l + border->l;
        NBK_gdi_setBrushColor(pfd, &rn->border_color_l);
        NBK_gdi_fillRect(pfd, &l);
    }

    if (rn->hasBrcolorT && border->t) {
        l = *rect;
        l.b = l.t + border->t;
        NBK_gdi_setBrushColor(pfd, &rn->border_color_t);
        NBK_gdi_fillRect(pfd, &l);
    }

    if (rn->hasBrcolorR && border->r) {
        l = *rect;
        l.l = l.r - border->r;
        NBK_gdi_setBrushColor(pfd, &rn->border_color_r);
        NBK_gdi_fillRect(pfd, &l);
    }

    if (rn->hasBrcolorB && border->b) {
        l = *rect;
        l.t = l.b - border->b;
        NBK_gdi_setBrushColor(pfd, &rn->border_color_b);
        NBK_gdi_fillRect(pfd, &l);
    }
}

void renderNode_drawBgImage(NRenderNode* rn, NRect* pr, NRect* vr, NStyle* style)
{
    NView* view = (NView*)style->view;
    NSize is;
    nbool fail;
    NCssValue bg_x, bg_y;
    
    if (rn->bgiid == -1 || !style->drawBgimg)
        return;

    if (imagePlayer_getSize(view->imgPlayer, rn->bgiid, &is, &fail)) {
        coord x, y;
        NImageDrawInfo di;
        
        di.id = rn->bgiid;

        di.r = rn->r;
        di.r.l += rn->margin.l + rn->border.l;
        di.r.t += rn->margin.t + rn->border.t;
        di.r.r -= rn->margin.r + rn->border.r;
        di.r.b -= rn->margin.b + rn->border.b;

        renderNode_getAbsPos(rn->parent, &x, &y);
        rect_move(&di.r, di.r.l + x, di.r.t + y);

        di.pr = pr;
        di.vr = vr;
        di.zoom = style->zoom;

        di.repeat = rn->bg_repeat;
        
        if (rn->bg_x_t == CSS_POS_PERCENT)
            cssVal_setPercent(&bg_x, rn->bg_x);
        else
            cssVal_setInt(&bg_x, rn->bg_x);
        
        if (rn->bg_y_t == CSS_POS_PERCENT)
            cssVal_setPercent(&bg_y, rn->bg_y);
        else
            cssVal_setInt(&bg_y, rn->bg_y);
        
        di.ox = css_calcBgPos(rect_getWidth(&di.r), is.w, bg_x,
            (rn->bg_repeat == CSS_REPEAT || rn->bg_repeat == CSS_REPEAT_X));
        di.oy = css_calcBgPos(rect_getHeight(&di.r), is.h, bg_y,
            (rn->bg_repeat == CSS_REPEAT || rn->bg_repeat == CSS_REPEAT_Y));

        di.iw = is.w;
        di.ih = is.h;

        imagePlayer_drawBg(view->imgPlayer, di);
    }
}

void renderNode_getSelPos(NRenderNode* rn, int* start, int* end)
{
    if (rn) {
        if (rn->type == RNT_INPUT)
            renderInput_getSelPos((NRenderInput*)rn, start, end);
        else if (rn->type == RNT_TEXTAREA)
            renderTextarea_getSelPos((NRenderTextarea*)rn, start, end);
    }
}

void renderNode_setSelPos(NRenderNode* rn, int start, int end)
{
    if (rn) {
        if (rn->type == RNT_INPUT)
            renderInput_setSelPos((NRenderInput*)rn, start, end);
        else if (rn->type == RNT_TEXTAREA)
            renderTextarea_setSelPos((NRenderTextarea*)rn, start, end);
    }
}

void rtree_traverse_depth(NRenderNode* root,
                          RTREE_TRAVERSE_START_CB startCb,
                          RTREE_TRAVERSE_CB closeCb,
                          void* user)
{
    NRenderNode* r = root;
    nbool ignore = N_FALSE;
    NRenderNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    
    while (r) {
        
        if (!ignore) {
            if (startCb && startCb(r, user, &ignore))
                break;
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            if (closeCb && !ignore && r->child == N_NULL) {
                if (closeCb(r, user))
                    break;
            }
            
            ignore = N_FALSE;
            
            if (r == root)
                break;
            else if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                if (closeCb && closeCb(r, user))
                    break;
                ignore = N_TRUE;
            }
        }
    }
}

NRenderNode* renderNode_getParent(NRenderNode* rn, uint8 type)
{
    NRenderNode* r = rn;
    while (r) {
        if (r->type == type)
            break;
        r = r->parent;
    }
    return r;
}

void renderNode_getAbsPos(NRenderNode* rn, coord* x, coord* y)
{
    NRenderNode* r = rn;
    *x = *y = 0;
    while (r) {
        if (r->type != RNT_INLINE) {
            *x += r->r.l + r->margin.l + r->border.l + r->padding.l;
            *y += r->r.t + r->margin.t + r->border.t + r->padding.t;
        }
        r = r->parent;
    }
}

void renderNode_drawFocusRing(NRect pr, NColor color, void* pfd)
{
    NRect l;
    coord bold = FOCUS_RING_BOLD;
    coord da = bold >> 1;
    
    NBK_gdi_setBrushColor(pfd, &color);
    
    l = pr;
    l.l -= da;
    l.r = l.l + bold;
    NBK_gdi_fillRect(pfd, &l);
    
    l = pr;
    l.t -= da;
    l.b = l.t + bold;
    NBK_gdi_fillRect(pfd, &l);
    
    l = pr;
    l.r += da;
    l.l = l.r - bold;
    NBK_gdi_fillRect(pfd, &l);
    
    l = pr;
    l.b += da;
    l.t = l.b - bold;
    NBK_gdi_fillRect(pfd, &l);
}

NRenderNode* rtree_get_last_sibling(NRenderNode* rn)
{
    NRenderNode* r = rn;
    while (r && r->next)
        r = r->next;
    return r;
}

#define IS_STRUCT_ELEMENT(r) (r->type == RNT_TEXT || r->type == RNT_INLINE)

NRenderNode* renderNode_getFirstChild(NRenderNode* rn)
{
    NRenderNode* sta[MAX_TREE_DEPTH];
    NRenderNode* r = rn->child;
    int lv = 0;
    nbool ignore = N_FALSE;

    sta[0] = rn;
    while (r) {
        if (!ignore) {
            if (!IS_STRUCT_ELEMENT(r))
                return r;
        }
        if (!ignore && IS_STRUCT_ELEMENT(r) && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == rn)
                break;
            else if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    return N_NULL;
}

NRenderNode* renderNode_getLastChild(NRenderNode* rn)
{
    NRenderNode* sta[MAX_TREE_DEPTH];
	NRenderNode* r;
    int lv = 0;
    nbool ignore = N_FALSE;

    sta[0] = rn;
    r = rtree_get_last_sibling(rn->child);
    while (r) {
        if (!ignore) {
            if (!IS_STRUCT_ELEMENT(r))
                return r;
        }
        if (!ignore && IS_STRUCT_ELEMENT(r) && r->child) {
            sta[++lv] = r;
            r = rtree_get_last_sibling(r->child);
        }
        else {
            ignore = N_FALSE;
            if (r == rn)
                break;
            if (r->prev)
                r = r->prev;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    return N_NULL;
}

static int build_stack(NRenderNode* sta[], NRenderNode* rn)
{
    int i = MAX_TREE_DEPTH;
    int j = -1;
    NRenderNode* r = rn->parent;

    while (r) {
        sta[--i] = r;
        if (IS_STRUCT_ELEMENT(r))
            r = r->parent;
        else
            break;
    }

    for (; i < MAX_TREE_DEPTH; i++)
        sta[++j] = sta[i];

    return j;
}

// 在包容块中获取上一个结点
NRenderNode* renderNode_getPrevNode(NRenderNode* rn)
{
    NRenderNode* sta[MAX_TREE_DEPTH];
    NRenderNode* r;
    int lv;
    nbool ignore = N_TRUE;

    lv = build_stack(sta, rn);
    if (lv == -1)
        return N_NULL;
    r = rn;
    while (r) {
        if (!ignore) {
            if (!IS_STRUCT_ELEMENT(r))
                return r;
        }
        if (!ignore && IS_STRUCT_ELEMENT(r) && r->child) {
            sta[++lv] = r;
            r = rtree_get_last_sibling(r->child);
        }
        else {
            ignore = N_FALSE;
            if (r == sta[0])
                break;
            if (r->prev)
                r = r->prev;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }

    return N_NULL;
}

// 在包容块中获取下一个结点
NRenderNode* renderNode_getNextNode(NRenderNode* rn)
{
    NRenderNode* sta[MAX_TREE_DEPTH];
    NRenderNode* r;
    int lv;
    nbool ignore = N_TRUE;

    lv = build_stack(sta, rn);
    if (lv == -1)
        return N_NULL;
    r = rn;
    while (r) {
        if (!ignore) {
            if (!IS_STRUCT_ELEMENT(r))
                return r;
        }
        if (!ignore && IS_STRUCT_ELEMENT(r) && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == sta[0])
                break;
            if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }

    return N_NULL;
}

coord renderNode_getInnerWidth(NRenderNode* rn)
{
    coord w = rect_getWidth(&rn->r);
    w -= rn->margin.l + rn->margin.r \
        + rn->border.l + rn->border.r \
        + rn->padding.l + rn->padding.r;
    return w;
}

coord renderNode_getContainerWidth(NRenderNode* rn)
{
    NRenderNode* r = rn->parent;
    while (r->type == RNT_INLINE)
        r = r->parent;
    return renderNode_getInnerWidth(r);
}

coord renderNode_getLeftMBP(NRenderNode* rn)
{
    return rn->margin.l + rn->border.l + rn->padding.l;
}

coord renderNode_getTopMBP(NRenderNode* rn)
{
    return rn->margin.t + rn->border.t + rn->padding.t;
}

coord renderNode_getRightMBP(NRenderNode* rn)
{
    return rn->margin.r + rn->border.r + rn->padding.r;
}

coord renderNode_getBottomMBP(NRenderNode* rn)
{
    return rn->margin.b + rn->border.b + rn->padding.b;
}

NRect renderNode_getBorderBox(NRenderNode* rn)
{
    NRect box;
    box.l = rn->r.l + rn->margin.l;
    box.t = rn->r.t + rn->margin.t;
    box.r = rn->r.r - rn->margin.r;
    box.b = rn->r.b - rn->margin.b;
    return box;
}

NRect renderNode_getContentBox(NRenderNode* rn)
{
    NRect box;
    box.l = rn->r.l + rn->margin.l + rn->border.l;
    box.t = rn->r.t + rn->margin.t + rn->border.t;
    box.r = rn->r.r - rn->margin.r - rn->border.r;
    box.b = rn->r.b - rn->margin.b - rn->border.b;
    return box;
}

nbool renderNode_hasBorder(NRenderNode* rn)
{
    if (rn->border.l || rn->border.t || rn->border.r || rn->border.b)
        return N_TRUE;
    else
        return N_FALSE;
}

static void transform_to_inline(NRenderNode* rn)
{
    rn->type = RNT_INLINE;
    rn->init = 0;
    rn->force = 0;
    rn->Layout = renderInline_layout;
    rn->Paint = renderInline_paint;
}

static void transform_to_block(NRenderNode* rn)
{
    rn->type = RNT_BLOCK;
    rn->init = 0;
    rn->force = 0;
    rn->Layout = renderBlock_layout;
    rn->Paint = renderBlock_paint;
}

static void transform_to_inlineBlock(NRenderNode* rn)
{
    rn->type = RNT_INLINE_BLOCK;
    rn->init = 0;
    rn->force = 0;
    rn->Layout = renderInlineBlock_layout;
    rn->Paint = renderInlineBlock_paint;
}

void renderNode_transform(NRenderNode* rn, NStyle* style)
{
    if (!style->display)
        return;

    if (rn->type == RNT_INLINE) {
        if (   style->flo
            || style->display == CSS_DISPLAY_BLOCK
            || style->display == CSS_DISPLAY_INLINE_BLOCK )
            transform_to_inlineBlock(rn);
        else if (style->display == CSS_DISPLAY_BLOCK)
            transform_to_block(rn);
    }
    else if (rn->type == RNT_BLOCK) {
        if (style->flo || style->display == CSS_DISPLAY_INLINE_BLOCK)
            transform_to_inlineBlock(rn);
        else if (style->display == CSS_DISPLAY_INLINE)
            transform_to_inline(rn);
    }
}

nbool renderNode_isBlockElement(const NRenderNode* rn)
{
    switch (rn->type) {
    case RNT_BLOCK:
    case RNT_TABLE:
    case RNT_TR:
        return N_TRUE;
    default:
        return N_FALSE;
    }
}

NRect renderNode_getPaintArea(NRenderNode* rn)
{
    NRect r = rn->r;
    r.l += rn->margin.l;
    r.t += rn->margin.t;
    r.r -= rn->margin.r;
    r.b -= rn->margin.b;
    return r;
}

void renderNode_fillButton(const NRect* pr, void* pfd)
{
    NColor c;
    NRect box = *pr;
    coord half = rect_getHeight(pr) / 2;
    box.b = box.t + half;
    color_set(&c, 239, 239, 239, 255);
    NBK_gdi_setBrushColor(pfd, &c);
    NBK_gdi_fillRect(pfd, &box);
    box.t = box.b;
    box.b = pr->b;
    color_set(&c, 214, 214, 214, 255);
    NBK_gdi_setBrushColor(pfd, &c);
    NBK_gdi_fillRect(pfd, &box);
}

nbool renderNode_isRenderDisplay(const NRenderNode* rn)
{
    NNode* n = (NNode*)rn->node;
    NRenderNode* r;

    do {
        r = (NRenderNode*)n->render;
        if (r && r->display == CSS_DISPLAY_NONE)
            return N_FALSE;
        n = n->parent;
    } while (n);

    return N_TRUE;
}
