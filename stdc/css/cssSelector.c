/*
 * cssSelector.c
 *
 *  Created on: 2011-1-9
 *      Author: wuyulun
 */

#include "cssSelector.h"
#include "color.h"
#include "css_val.h"
#include "css_helper.h"
#include "css_prop.h"
#include "css_value.h"
#include "../inc/nbk_limit.h"
#include "../dom/xml_helper.h"
#include "../dom/xml_tags.h"
#include "../dom/page.h"
#include "../tools/dump.h"
#include "../tools/str.h"

#define DUMP_FUNCTIONS      0
#define DEBUG_PARSE         0

#ifdef NBK_MEM_TEST
int cssStyle_memUsed(const NCssStyle* style, nbool ownBg)
{
    int size = 0;
    if (style) {
        size += sizeof(NCssStyle);
        if (ownBg && style->bgImage) {
            size += nbk_strlen(style->bgImage) + 1;
        }
    }
    return size;
}

int sheet_memUsed(const NSheet* sheet)
{
    int size = 0;
    int i, bytes, total;
    
    if (sheet) {
        NPage* page = (NPage*)sheet->page;
        
        size += sizeof(NSheet);
        
        bytes = total = 0;
        for (i=0; i < TAGID_LASTTAG; i++) {
            if (sheet->ts[i]) {
                total++;
                bytes += cssStyle_memUsed(sheet->ts[i], N_FALSE);
            }
        }
        size += bytes;
        dump_char(page, "univerial selector", -1);
        dump_int(page, total);
        dump_int(page, bytes);
        dump_return(page);
        
        bytes = 0;
        for (i=0; i < sheet->cs->use; i++) {
            NSDCell* c = &sheet->cs->a[i];
            bytes += cssStyle_memUsed((NCssStyle*)c->v, N_FALSE);
        }
        size += bytes;
        dump_char(page, "class selector", -1);
        dump_int(page, sheet->cs->use);
        dump_int(page, bytes);
        dump_return(page);
        
        bytes = 0;
        for (i=0; i < sheet->is->use; i++) {
            NSDCell* c = &sheet->is->a[i];
            bytes += cssStyle_memUsed((NCssStyle*)c->v, N_FALSE);
        }
        size += bytes;
        dump_char(page, "id selector", -1);
        dump_int(page, sheet->is->use);
        dump_int(page, bytes);
        dump_return(page);
        
        bytes = total = 0;
        {
            NSelCell* c = sll_first(sheet->ds);
            NSelCell* p;
            while (c) {
                p = c;
                total++;
                bytes += sizeof(NSelCell);
                if (!p->hasChild)
                    bytes += cssStyle_memUsed(p->d.style, N_FALSE);
                while (p->hasChild) {
                    p = p->d.child;
                    total++;
                    bytes += sizeof(NSelCell);
                    if (!p->hasChild)
                        bytes += cssStyle_memUsed(p->d.style, N_FALSE);
                }
                c = sll_next(sheet->ds);
            }
        }
        size += bytes;
        dump_char(page, "descendant selector", -1);
        dump_int(page, total);
        dump_int(page, bytes);
        dump_return(page);
        
        bytes = strPool_memUsed(sheet->strPool);
        size += bytes;
        dump_char(page, "string pool", -1);
        dump_int(page, bytes);
        dump_return(page);
        
        bytes = 0;
        for (i=0; i < sheet->bgimgs->use; i++)
            bytes += nbk_strlen((char*)sheet->bgimgs->data[i]) + 1;
        size += bytes;
        dump_char(page, "background urls", -1);
        dump_int(page, sheet->bgimgs->use);
        dump_int(page, bytes);
        dump_return(page);
    }
    
    return size;
}
#endif

static void sh_clear_ts(NCssStyle* sh[])
{
    int i;
    for (i=0; i < TAGID_LASTTAG; i++)
        if (sh[i]) {
            NBK_free(sh[i]);
            sh[i] = N_NULL;
        }
}

static void sh_clear_cs(NStrDict* dict)
{
    int i;
    NSDCell* c;
    for (i=0; i < dict->use; i++) {
        c = &dict->a[i];
        NBK_free(c->v);
    }
    strDict_reset(dict);
}

static void sh_clear_ds(NSList* ds)
{
    NSelCell* c = (NSelCell*)sll_first(ds);
    NSelCell *p, *t;
    
    while (c) {
        p = c;
        while (p->hasChild) {
            t = p;
            p = p->d.child;
            NBK_free(t);
        }
        NBK_free(p->d.style);
        NBK_free(p);
        c = (NSelCell*)sll_next(ds);
    }
    sll_reset(ds);
}

static void sh_style_init(NCssStyle* style)
{
    style->display = CSS_DISPLAY_UNSET;
    style->flo = CSS_FLOAT_UNSET;
    
    style->bg_x_t = style->bg_y_t = CSS_POS_PIXEL;
    style->bg_x = style->bg_y = N_INVALID_COORD;
    
    style->opacity = 1.0f;
    
    style->margin.l = style->margin.t = style->margin.r = style->margin.b = N_INVALID_COORD;
    style->border.l = style->border.t = style->border.r = style->border.b = N_INVALID_COORD;
    style->padding.l = style->padding.t = style->padding.r = style->padding.b = N_INVALID_COORD;
}

static void sh_delete_selcell(NSelCell** cell)
{
    NSelCell* c = *cell;
    NSelCell* t;
    
    while (c) {
        t = c;
        c = c->d.child;
        NBK_free(t);
    }
    *cell = N_NULL;
}

NSheet* sheet_create(void* page)
{
    NCssStyle* style;
    NSheet* sheet = (NSheet*)NBK_malloc0(sizeof(NSheet));
    N_ASSERT(sheet);
    
    // default style
    style = (NCssStyle*)NBK_malloc0(sizeof(NCssStyle));
    N_ASSERT(style);
    
    sh_style_init(style);
    sheet->ts[0] = style;
    
    sheet->cs = strDict_create(10);
    sheet->is = strDict_create(10);
    sheet->ds = sll_create();
    
    sheet->strPool = strPool_create();
    sheet->bgimgs = ptrArray_create();
    
    sheet->page = page;
    
    return sheet;
}

void sheet_delete(NSheet** sheet)
{
    NSheet* sh = *sheet;
    sheet_reset(sh);
    strDict_delete(&sh->cs);
    strDict_delete(&sh->is);
    sll_delete(&sh->ds);
    strPool_delete(&sh->strPool);
    ptrArray_delete(&sh->bgimgs);
    NBK_free(sh);
    *sheet = N_NULL;
}

void sheet_reset(NSheet* sheet)
{
    int i;
    
    sh_clear_ts(sheet->ts);
    sh_clear_cs(sheet->cs);
    sh_clear_cs(sheet->is);
    sh_clear_ds(sheet->ds);
    strPool_reset(sheet->strPool);
    
    for (i=0; i < sheet->bgimgs->use; i++)
        NBK_free(sheet->bgimgs->data[i]);
    ptrArray_reset(sheet->bgimgs);
    
    if (sheet->data)
        strBuf_delete(&sheet->data);
}

static nbool sh_parse_border_color_short(NCssStyle* cs, NCssValue vals[], int num)
{
    NColor colors[4];
    int i, j;
    
    for (i=j=0; i < num && i < 4; i++) {
        if (vals[i].type == NECVT_COLOR) {
            colors[j++] = vals[i].d.color;
        }
        else if (vals[i].type == NECVT_VALUE) {
            if (vals[i].d.value == CSSVAL_SOLID)
                cs->hasBorder = 1;
        }
    }
    
    switch (j) {
    case 1:
        cs->border_color_l = cs->border_color_t = cs->border_color_r = cs->border_color_b = colors[0];
        cs->hasBrcolorL = cs->hasBrcolorT = cs->hasBrcolorR = cs->hasBrcolorB = 1;
        break;
    case 2:
        cs->border_color_t = cs->border_color_b = colors[0];
        cs->border_color_l = cs->border_color_r = colors[1];
        cs->hasBrcolorL = cs->hasBrcolorT = cs->hasBrcolorR = cs->hasBrcolorB = 1;
        break;
    case 3:
        cs->border_color_t = colors[0];
        cs->border_color_l = cs->border_color_r = colors[1];
        cs->border_color_b = colors[2];
        cs->hasBrcolorL = cs->hasBrcolorT = cs->hasBrcolorR = cs->hasBrcolorB = 1;
        break;
    case 4:
        cs->border_color_t = colors[0];
        cs->border_color_r = colors[1];
        cs->border_color_b = colors[2];
        cs->border_color_l = colors[3];
        cs->hasBrcolorL = cs->hasBrcolorT = cs->hasBrcolorR = cs->hasBrcolorB = 1;
        break;
    }
    
    return (nbool)j;
}

static nbool sh_parse_box_short(NCssBox* box, NCssValue vals[], int num)
{
    coord w[4];
    int i, j;
    
    for (i=j=0; i < num && i < 4; i++) {
        if (vals[i].type == NECVT_INT) {
            w[j++] = (coord)vals[i].d.i32;
        }
    }
    
    switch (j) {
    case 1:
        box->l = box->t = box->r = box->b = w[0];
        break;
    case 2:
        box->t = box->b = w[0];
        box->l = box->r = w[1];
        break;
    case 3:
        box->t = w[0];
        box->r = box->l = w[1];
        box->b = w[2];
        break;
    case 4:
        box->t = w[0];
        box->r = w[1];
        box->b = w[2];
        box->l = w[3];
        break;
    }
    
    return (nbool)j;
}

static nbool sh_parse_box_side(coord* side, NColor* color, NCssValue vals[], int num)
{
    int i;
    nbool valid = (color) ? N_FALSE : N_TRUE;
    
    for (i=0; i < num; i++) {
        if (color && vals[i].type == NECVT_COLOR)
            *color = vals[i].d.color;
        else if (vals[i].type == NECVT_INT)
            *side = (coord)vals[i].d.i32;
        else if (vals[i].type == NECVT_VALUE
            && (   vals[i].d.value == CSSVAL_SOLID
                || vals[i].d.value == CSSVAL_DASHED
                || vals[i].d.value == CSSVAL_DOTTED) )
            valid = N_TRUE;
    }
    
    return valid;
}

static void sh_parse_background(NCssStyle* cs, NCssValue vals[], int num)
{
    int i;
    
    for (i=0; i < num; i++) {
        
        switch (vals[i].type) {
        case NECVT_COLOR:
            cs->background_color = vals[i].d.color;
            cs->hasBgcolor = 1;
            break;
            
        case NECVT_URL:
            if (cs->bgImage) // 当样式中包含重复url时，使用最后一个
                NBK_free(cs->bgImage);
            cs->bgImage = vals[i].d.url;
            break;
        
        case NECVT_VALUE:
        {
            switch (vals[i].d.value) {
            case CSSVAL_NO_REPEAT:
                cs->bg_repeat = CSS_REPEAT_NO;
                break;
            case CSSVAL_REPEAT_X:
                cs->bg_repeat = CSS_REPEAT_X;
                break;
            case CSSVAL_REPEAT_Y:
                cs->bg_repeat = CSS_REPEAT_Y;
                break;
            case CSSVAL_LEFT:
                cs->bg_x_t = CSS_POS_PERCENT;
                cs->bg_x = 0;
                break;
            case CSSVAL_RIGHT:
                cs->bg_x_t = CSS_POS_PERCENT;
                cs->bg_x = 100;
                break;
            case CSSVAL_TOP:
                cs->bg_y_t = CSS_POS_PERCENT;
                cs->bg_y = 0;
                break;
            case CSSVAL_BOTTOM:
                cs->bg_y_t = CSS_POS_PERCENT;
                cs->bg_y = 100;
                break;
            case CSSVAL_CENTER:
                if (cs->bg_x == N_INVALID_COORD) {
                    cs->bg_x_t = CSS_POS_PERCENT;
                    cs->bg_x = 50;
                }
                else {
                    cs->bg_y_t = CSS_POS_PERCENT;
                    cs->bg_y = 50;
                }
                break;
            }
            break;
        }
        
        case NECVT_INT:
            if (cs->bg_x == N_INVALID_COORD) {
                cs->bg_x = (coord)vals[i].d.i32;
                cs->bg_x_t = CSS_POS_PIXEL;
            }
            else {
                cs->bg_y = (coord)vals[i].d.i32;
                cs->bg_y_t = CSS_POS_PIXEL;
            }
            break;
        
        case NECVT_PERCENT:
            if (cs->bg_x == N_INVALID_COORD) {
                cs->bg_x = vals[i].d.perc;
                cs->bg_x_t = CSS_POS_PERCENT;
            }
            else {
                cs->bg_y = vals[i].d.perc;
                cs->bg_y_t = CSS_POS_PERCENT;
            }
            break;
        } // switch
    } // for
}

NCssStyle* sheet_parseInlineStyle(const char* style)
{
    NCssStyle* cs;
    NCssDecl decl[MAX_CSS_DECL];
    NCssValue vals[MAX_CSS_VALUE];
    int i, j, num, vn;
    nid id;
    
    cs = (NCssStyle*)NBK_malloc0(sizeof(NCssStyle));
    N_ASSERT(cs);
    sh_style_init(cs);
    
    num = css_parseDecl(style, -1, decl, MAX_CSS_DECL);
    
#if DEBUG_PARSE
    for (i=0; i < num; i++) {
        dump_int(g_dp, i);
        dump_char(g_dp, decl[i].p.s, decl[i].p.l);
        dump_char(g_dp, ":", -1);
        dump_char(g_dp, decl[i].v.s, decl[i].v.l);
        dump_return(g_dp);
    }
#endif
    
    for (i=0; i < num; i++) {
        
        if (decl[i].p.l == 0 || decl[i].v.l == 0)
            continue;
        
        str_toLower(decl[i].p.s, decl[i].p.l);
        id = css_getPropertyId(decl[i].p.s, decl[i].p.l);
        if (id == N_INVALID_ID)
            continue;
        
        vn = css_parseValues(decl[i].v.s, decl[i].v.l, vals, MAX_CSS_VALUE);
        
        switch (id) {
        case CSSID_FLOAT:
            if (vn == 1 && vals[0].type == NECVT_VALUE) {
                if      (vals[0].d.value == CSSVAL_NONE)
                    cs->flo = CSS_FLOAT_NONE;
                else if (vals[0].d.value == CSSVAL_LEFT)
                    cs->flo = CSS_FLOAT_LEFT;
                else if (vals[0].d.value == CSSVAL_RIGHT)
                    cs->flo = CSS_FLOAT_RIGHT;
            }
            break;
            
        case CSSID_CLEAR:
            if (vals[0].type == NECVT_VALUE) {
                if (vals[0].d.value == CSSVAL_BOTH)
                    cs->clr = CSS_CLEAR_BOTH;
            }
            break;
            
        case CSSID_COLOR:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_COLOR) {
                    cs->color = vals[j].d.color;
                    cs->hasColor = 1;
                    break;
                }
            }
            break;
            
        case CSSID_BACKGROUND:
        case CSSID_BACKGROUND_IMAGE:
        case CSSID_BACKGROUND_COLOR:
        case CSSID_BACKGROUND_POSITION:
        case CSSID_BACKGROUND_REPEAT:
            sh_parse_background(cs, vals, vn);
            break;
            
        case CSSID_MARGIN:
            if (sh_parse_box_short(&cs->margin, vals, vn))
                cs->hasMargin = 1;
            break;
            
        case CSSID_MARGIN_LEFT:
            if (sh_parse_box_side(&cs->margin.l, N_NULL, vals, vn))
                cs->hasMargin = 1;
            break;
            
        case CSSID_MARGIN_TOP:
            if (sh_parse_box_side(&cs->margin.t, N_NULL, vals, vn))
                cs->hasMargin = 1;
            break;
            
        case CSSID_MARGIN_RIGHT:
            if (sh_parse_box_side(&cs->margin.r, N_NULL, vals, vn))
                cs->hasMargin = 1;
            break;
            
        case CSSID_MARGIN_BOTTOM:
            if (sh_parse_box_side(&cs->margin.b, N_NULL, vals, vn))
                cs->hasMargin = 1;
            break;
            
        case CSSID_BORDER:
            if (sh_parse_box_short(&cs->border, vals, vn)) {
                for (j=0; j < vn; j++) {
                    if (   vals[j].type == NECVT_VALUE
                        && (   vals[j].d.value == CSSVAL_SOLID
                            || vals[j].d.value == CSSVAL_DASHED ) ) {
                        cs->hasBorder = 1;
                    }
                    else if (vals[j].type == NECVT_COLOR) {
                        cs->border_color_l = vals[j].d.color;
                        cs->border_color_t = vals[j].d.color;
                        cs->border_color_r = vals[j].d.color;
                        cs->border_color_b = vals[j].d.color;
                        cs->hasBrcolorL = 1;
                        cs->hasBrcolorT = 1;
                        cs->hasBrcolorR = 1;
                        cs->hasBrcolorB = 1;
                    }
                }
            }
            break;
            
        case CSSID_BORDER_COLOR:
            sh_parse_border_color_short(cs, vals, vn);
            break;
            
        case CSSID_BORDER_LEFT:
            if (sh_parse_box_side(&cs->border.l, &cs->border_color_l, vals, vn)) {
                cs->hasBorder = 1;
                cs->hasBrcolorL = 1;
            }
            break;
            
        case CSSID_BORDER_TOP:
            if (sh_parse_box_side(&cs->border.t, &cs->border_color_t, vals, vn)) {
                cs->hasBorder = 1;
                cs->hasBrcolorT = 1;
            }
            break;
            
        case CSSID_BORDER_RIGHT:
            if (sh_parse_box_side(&cs->border.r, &cs->border_color_r, vals, vn)) {
                cs->hasBorder = 1;
                cs->hasBrcolorR = 1;
            }
            break;
            
        case CSSID_BORDER_BOTTOM:
            if (sh_parse_box_side(&cs->border.b, &cs->border_color_b, vals, vn)) {
                cs->hasBorder = 1;
                cs->hasBrcolorB = 1;
            }
            break;
            
        case CSSID_PADDING:
            if (sh_parse_box_short(&cs->padding, vals, vn))
                cs->hasPadding = 1;
            break;
            
        case CSSID_PADDING_LEFT:
            if (sh_parse_box_side(&cs->padding.l, N_NULL, vals, vn))
                cs->hasPadding = 1;
            break;
            
        case CSSID_PADDING_TOP:
            if (sh_parse_box_side(&cs->padding.t, N_NULL, vals, vn))
                cs->hasPadding = 1;
            break;
            
        case CSSID_PADDING_RIGHT:
            if (sh_parse_box_side(&cs->padding.r, N_NULL, vals, vn))
                cs->hasPadding = 1;
            break;
            
        case CSSID_PADDING_BOTTOM:
            if (sh_parse_box_side(&cs->padding.b, N_NULL, vals, vn))
                cs->hasPadding = 1;
            break;
            
        case CSSID_FONT_SIZE:
            if (vn >= 1 && vals[0].type)
                cs->font_size = vals[0];
            break;
            
        case CSSID_FONT_WEIGHT:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_VALUE && vals[j].d.value == CSSVAL_BOLD)
                    cs->font_weight = CSS_FONT_BOLD;
            }
            break;
            
        case CSSID_TEXT_ALIGN:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_VALUE) {
                    if (vals[j].d.value == CSSVAL_RIGHT)
                        cs->text_align = CSS_ALIGN_RIGHT;
                    else if (vals[j].d.value == CSSVAL_CENTER)
                        cs->text_align = CSS_ALIGN_CENTER;
                    else
                        cs->text_align = CSS_ALIGN_LEFT;
                    cs->hasTextAlign = 1;
                }
            }
            break;
            
        case CSSID_TEXT_DECORATION:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_VALUE) {
                    if (vals[j].d.value == CSSVAL_UNDERLINE)
                        cs->text_decor = CSS_TXTDECOR_UNDERLINE;
                    cs->hasTextDecor = 1;
                }
            }
            break;
            
        case CSSID_TEXT_INDENT:
            if (vn >= 1 && vals[0].type)
                cs->text_indent = vals[0];
            break;
            
        case CSSID_LINE_HEIGHT:
            if (vn >= 1 && vals[0].type)
                cs->line_height = vals[0];
            break;
            
        case CSSID_OVERFLOW:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_VALUE) {
                    if (vals[j].d.value == CSSVAL_HIDDEN)
                        cs->overflow = CSS_OVERFLOW_HIDDEN;
                    cs->hasOverflow = 1;
                }
            }
            break;
            
        case CSSID_VERTICAL_ALIGN:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_VALUE) {
                    if (vals[j].d.value == CSSVAL_TOP)
                        cs->vert_align = CSS_VALIGN_TOP;
                    else if (vals[j].d.value == CSSVAL_MIDDLE)
                        cs->vert_align = CSS_VALIGN_MIDDLE;
                    else
                        cs->vert_align = CSS_VALIGN_BOTTOM;
                    cs->hasVertAlign = 1;
                }
            }
            break;
            
        case CSSID_DISPLAY:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_VALUE) {
                    switch (vals[j].d.value) {
                    case CSSVAL_NONE:
                        cs->display = CSS_DISPLAY_NONE;
                        break;
                    case CSSVAL_INLINE:
                        cs->display = CSS_DISPLAY_INLINE;
                        break;
                    case CSSVAL_BLOCK:
                        cs->display = CSS_DISPLAY_BLOCK;
                        break;
                    case CSSVAL_INLINE_BLOCK:
                        cs->display = CSS_DISPLAY_INLINE_BLOCK;
                        break;
                    default:
                        cs->display = CSS_DISPLAY_UNSET;
                        break;
                    }
                }
            }
            break;
            
        case CSSID_Z_INDEX:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_INT) {
                    cs->z_index = vals[j].d.i32;
                }
            }
            break;
            
        case CSSID_WIDTH:
            if (vn == 1)
                cs->width = vals[0];
            break;
            
        case CSSID_HEIGHT:
            if (vn == 1)
                cs->height = vals[0];
            break;
            
        case CSSID_MAX_WIDTH:
            if (vn == 1)
                cs->max_width = vals[0];
            break;
            
        case CSSID_OPACITY:
            for (j=0; j < vn; j++) {
                if (vals[j].type == NECVT_FLOAT) {
                    cs->opacity = vals[j].d.flo;
                    cs->hasOpacity = 1;
                }
            }
            break;
        } // switch
    } // for
    
    // verify style
    
    if (!cs->hasBorder) {
        NBK_memset(&cs->border, 0, sizeof(NCssBox));
    }
    if (cs->bgImage) {
        cs->bg_x = (cs->bg_x == N_INVALID_COORD) ? 0 : cs->bg_x;
        cs->bg_y = (cs->bg_y == N_INVALID_COORD) ? 0 : cs->bg_y;
    }
    
    return cs;
}

static nbool sh_verify_selcell(NSelCell* sc)
{
    nbool pass = N_TRUE;
    NSelCell* s = sc;
    NSelCell* last = N_NULL;
    
    while (s) {
        if (s->tid == N_INVALID_ID && s->txt == N_NULL) {
            pass = N_FALSE;
            break;
        }
        if (s->type == SELT_UNUSED) {
            NBK_free(last->d.child);
            last->hasChild = N_FALSE;
            last->d.child = N_NULL;
            break;
        }
        if (s->hasChild) {
            last = s;
            s = s->d.child;
        }
        else
            break;
    }
    
    if (!pass) {
        if (sc->hasChild)
            sh_delete_selcell(&sc->d.child);
        NBK_memset(sc, 0, sizeof(NSelCell));
    }
    
    return pass;
}

static int sh_parse_selector(
    NStrPool* pool, char* sel, int length, NSelCell lst[], int max)
{
    char* p = sel;
    char* toofar = sel + length;
    char *t, *n;
    int i = 0, len;
    char tag[MAX_TAG_LEN + 1];
    nbool atDeli, atEnd;
    NSelCell *cell;
    nid id;
    
    t = n = N_NULL;
    cell = &lst[0];
    cell->type = SELT_UNUSED;
    while (p <= toofar && i < max) {
        
        atEnd = (p == toofar);        
        atDeli = atEnd || (*p <= ' ' || *p == ',');
        if (t && (atDeli || *p == '.' || *p =='#')) {
            len = p - t;
#if DEBUG_PARSE
            dump_char(g_dp, "tag ->", -1);
            dump_char(g_dp, t, len);
            dump_return(g_dp);
#endif
            if (len > MAX_TAG_LEN) {
                id = 0;
            }
            else {
                nbk_strncpy(tag, t, len);
                str_toLower(tag, len);
                id = get_tag_id(tag);
            }
            t = N_NULL;
            cell->tid = (id == 0) ? N_INVALID_ID : id;
            cell->type = SELT_TAG;
        }
        if (n && atDeli) {
#if DEBUG_PARSE
            dump_char(g_dp, "lab ->", -1);
            dump_char(g_dp, n, p - n);
            dump_return(g_dp);
#endif
            cell->tid = (cell->tid == N_INVALID_ID) ? 0 : cell->tid;
            cell->txt = strPool_save(pool, n, p - n);
            n = N_NULL;
        }
        if (atEnd) {
            if (sh_verify_selcell(&lst[i]))
                i++;
            break;
        }
        
        switch (*p) {
        case '.':
            cell->type = (cell->type == SELT_TAG) ? SELT_TAG_CLASS : SELT_CLASS;
            n = p + 1;
            break;
            
        case '#':
            cell->type = (cell->type == SELT_TAG) ? SELT_TAG_ID : SELT_ID;
            n = p + 1;
            break;
            
        case '*':
            NBK_memset(cell, 0, sizeof(NSelCell));
            cell->type = SELT_ALL;
            break;
            
        case ',':
            if (sh_verify_selcell(&lst[i]))
                i++;
			if (i < max) {
				cell = &lst[i];
				cell->type = SELT_UNUSED;
			}
            break;
            
        case ' ':
        case '\n':
        case '\r':
        case '\t':
            if (cell->type != SELT_UNUSED) {
                cell->hasChild = N_TRUE;
                cell->d.child = (NSelCell*)NBK_malloc0(sizeof(NSelCell));
                cell = cell->d.child;
                cell->type = SELT_UNUSED;
            }
            break;
            
        default:
            if (t == N_NULL && n == N_NULL)
                t = p;
            break;
        }
        
        p++;
    }
    
#if DEBUG_PARSE
    dump_char(g_dp, "NUM ->", -1);
    dump_int(g_dp, i);
    dump_return(g_dp);
#endif
    return i;
}

// 建立样式数据表
static void sh_build_sheet(NSheet* sheet, NSelCell lst[], int num, NCssStyle* cs)
{
    int i, j, sz1 = sizeof(NCssStyle), sz2 = sizeof(NSelCell);
    NSelCell *c, *nc;
    NCssStyle* s;
    NCssStyle* t = N_NULL;
    
    for (i=0; i < num; i++) {
        
        c = &lst[i];
        
        if (c->tid == N_INVALID_ID)
            continue;
        
        if (c->type == SELT_ALL) {
            if (sheet->ts[0])
                css_addStyle(sheet->ts[0], cs);
            else {
                sheet->ts[0] = (NCssStyle*)NBK_malloc(sz1);
                NBK_memcpy(sheet->ts[0], cs, sz1);
            }
            // 应用于预置样式
            for (j=1; j < TAGID_LASTTAG; j++) {
                if (sheet->ts[j])
                    css_addStyle(sheet->ts[j], cs);
            }
        }
        else if (c->type == SELT_TAG && !c->hasChild) {
            // type selector

            if (t == N_NULL)
                t = (NCssStyle*)NBK_malloc(sz1);
            NBK_memcpy(t, cs, sz1);
            
            if (sheet->ts[c->tid])
                css_addStyle(sheet->ts[c->tid], t);
            else {
                sheet->ts[c->tid] = t;
                t = N_NULL;
            }
        }
        else if (c->type == SELT_CLASS && !c->hasChild) {
            // class selector
            
            if (t == N_NULL)
                t = (NCssStyle*)NBK_malloc(sz1);
            NBK_memcpy(t, cs, sz1);
            
            if ((s = (NCssStyle*)strDict_append(sheet->cs, c->txt, t)) != N_NULL)
                css_addStyle(s, t);
            else
                t = N_NULL;
        }
        else if (c->type == SELT_ID && !c->hasChild) {
            // id selector
            
            if (t == N_NULL)
                t = (NCssStyle*)NBK_malloc(sz1);
            NBK_memcpy(t, cs, sz1);
            
            if ((s = (NCssStyle*)strDict_append(sheet->is, c->txt, t)) != N_NULL)
                css_addStyle(s, t);
            else
                t = N_NULL;
        }
        else {
            // descendant selector or other ( tag.class tag#id )
            
            nc = (NSelCell*)NBK_malloc(sz2);
            NBK_memcpy(nc, c, sz2);
            sll_append(sheet->ds, nc);
            while (nc->hasChild)
                nc = nc->d.child;
            
            if (t == N_NULL)
                t = (NCssStyle*)NBK_malloc(sz1);
            NBK_memcpy(t, cs, sz1);
            
            nc->d.style = t;
            c->d.child = N_NULL;
            t = N_NULL;
        }
    }
    
    if (t)
        NBK_free(t);
}

#if DUMP_FUNCTIONS
static void sheet_dump_cssstyle_header(void)
{
    dump_char(g_dp, "*", 1);

    dump_char(g_dp, "width", -1);
    dump_char(g_dp, "height", -1);
    
    dump_char(g_dp, "float", -1);
    dump_char(g_dp, "display", -1);
    
    dump_char(g_dp, "font-size", -1);
    dump_char(g_dp, "font-weight", -1);
    dump_char(g_dp, "text-align", -1);
    dump_char(g_dp, "line-height", -1);

    dump_char(g_dp, "color", -1);
    dump_char(g_dp, "bg-color", -1);
    dump_char(g_dp, "border-l", -1);
    dump_char(g_dp, "border-t", -1);
    dump_char(g_dp, "border-r", -1);
    dump_char(g_dp, "border-b", -1);

    dump_char(g_dp, "margin", -1);
    dump_char(g_dp, "border", -1);
    dump_char(g_dp, "padding", -1);
    
    dump_char(g_dp, "bg-image", -1);
    
    dump_return(g_dp);
}

static void sheet_dump_cssstyle(NCssStyle* cs)
{
    dump_NCssValue(g_dp, &cs->width);
    dump_NCssValue(g_dp, &cs->height);
    
    dump_uint(g_dp, cs->flo);
    dump_uint(g_dp, cs->display);
    
    dump_NCssValue(g_dp, &cs->font_size);
    
    dump_uint(g_dp, cs->font_weight);
    dump_uint(g_dp, cs->text_align);
    dump_NCssValue(g_dp, &cs->line_height);
    
    // color
    (cs->hasColor) ? dump_NColor(g_dp, &cs->color) : dump_char(g_dp, "-", 1);
    (cs->hasBgcolor) ? dump_NColor(g_dp, &cs->background_color) : dump_char(g_dp, "-", 1);
    (cs->hasBrcolorL) ? dump_NColor(g_dp, &cs->border_color_l) : dump_char(g_dp, "-", 1);
    (cs->hasBrcolorT) ? dump_NColor(g_dp, &cs->border_color_t) : dump_char(g_dp, "-", 1);
    (cs->hasBrcolorR) ? dump_NColor(g_dp, &cs->border_color_r) : dump_char(g_dp, "-", 1);
    (cs->hasBrcolorB) ? dump_NColor(g_dp, &cs->border_color_b) : dump_char(g_dp, "-", 1);
    
    // box
    (cs->hasMargin) ? dump_NCssBox(g_dp, &cs->margin) : dump_char(g_dp, "-", 1);
    (cs->hasBorder) ? dump_NCssBox(g_dp, &cs->border) : dump_char(g_dp, "-", 1);
    (cs->hasPadding) ? dump_NCssBox(g_dp, &cs->padding) : dump_char(g_dp, "-", 1);
    
    (cs->bgImage) ? dump_char(g_dp, cs->bgImage, -1) : dump_char(g_dp, "-", -1);
    
    dump_return(g_dp);
}

static void sheet_dump_selcell(NSelCell* sc)
{
    dump_char(g_dp, (char*)xml_getTagNames()[sc->tid], -1);
    
    switch (sc->type) {
    case SELT_TAG:
        dump_char(g_dp, "tag", -1);
        break;
    case SELT_CLASS:
        dump_char(g_dp, "class", -1);
        break;
    case SELT_ID:
        dump_char(g_dp, "id", -1);
        break;
    }
    
    dump_char(g_dp, ((sc->txt) ? sc->txt : " "), -1);
    dump_return(g_dp);
    
    if (!sc->hasChild) {
        dump_char(g_dp, " ", 1);
        sheet_dump_cssstyle(sc->d.style);
    }
}

static void sheet_dump_dict(NStrDict* dic)
{
    int i;
    
    for (i=0; i < dic->use; i++) {
        dump_char(g_dp, dic->a[i].k, -1);
        sheet_dump_cssstyle((NCssStyle*)dic->a[i].v);
    }
}
#endif

void sheet_dump(NSheet* sheet)
{
#if DUMP_FUNCTIONS
    int i;
    NSelCell* sc;
    
    sheet_dump_cssstyle_header();
    
    dump_char(g_dp, "Universal Selector", -1);
    dump_return(g_dp);
    for (i=0; i < TAGID_LASTTAG; i++) {
        if (sheet->ts[i]) {
            dump_char(g_dp, (char*)xml_getTagNames()[i], -1);
            sheet_dump_cssstyle(sheet->ts[i]);
        }
    }
    
    dump_char(g_dp, "Class Selector", -1);
    dump_return(g_dp);
    sheet_dump_dict(sheet->cs);
    
    dump_char(g_dp, "ID Selector", -1);
    dump_return(g_dp);
    sheet_dump_dict(sheet->is);
    
    dump_char(g_dp, "Other selector", -1);
    dump_return(g_dp);
    sc = sll_first(sheet->ds);
    while (sc) {
        i = 1;
        while(sc) {
            dump_int(g_dp, i++);
            sheet_dump_selcell(sc);
            if (sc->hasChild)
                sc = sc->d.child;
            else
                break;
        }
        sc = sll_next(sheet->ds);
    }
    
    dump_flush(g_dp);
#endif
}

void sheet_parseStyle(NSheet* sheet, const char* style, int length)
{
    char *p, *toofar;
    char *sel, *sty;
    char t;
    NSelCell lst[MAX_SELECTOR_NUM];
    int i, num = 0;
    NCssStyle* cs = N_NULL;
    nbool comment = N_FALSE;
    nbool at_rule = N_FALSE;
    int lv = 0;
    
    p = (char*)style;
    toofar = (length == -1) ? (char*)N_MAX_UINT : (char*)style + length;
    
    sel = sty = N_NULL;
    t = 0;
    while (*p && p < toofar) {
        
        if (comment) {
            if (*p == '*' && *(p+1) == '/') {
                comment = N_FALSE;
                p += 2;
            }
            else
                p++;
            continue;
        }
        else {
            if (*p == '/' && *(p+1) == '*') {
                comment = N_TRUE;
                p += 2;
                continue;
            }
        }
        
        if (at_rule) {
            if (*p == ';')
                at_rule = N_FALSE;
            p++;
            continue;
        }
        else {
            if (*p == '@') {
                at_rule = N_TRUE;
                p++;
                continue;
            }
        }
        
        if (sel == N_NULL && *p <= ' ') {
            p++;
            continue;
        }
        if (sel == N_NULL) {
            sel = p;
            t = '{';
            p++;
            continue;
        }
        
        if (*p == t) {
            if (t == '{') {
#if DEBUG_PARSE
                dump_return(g_dp);
                dump_char(g_dp, "SEL", -1);
                dump_char(g_dp, sel, p - sel);
                dump_return(g_dp);
#endif
                NBK_memset(lst, 0, sizeof(NSelCell) * MAX_SELECTOR_NUM);
                num = sh_parse_selector(sheet->strPool, sel, p - sel, lst, MAX_SELECTOR_NUM);
                t = '}';
                sty = p + 1;
            }
            else { // '}'
                if (lv)
                    lv--;
                else {
                    *p = 0;
#if DEBUG_PARSE
                    dump_char(g_dp, sty, -1);
                    dump_return(g_dp);
#endif
                    cs = sheet_parseInlineStyle(sty);
                    if (cs->bgImage)
                        ptrArray_append(sheet->bgimgs, cs->bgImage);
                    *p = '}';
                    sel = N_NULL;
                    
                    sh_build_sheet(sheet, lst, num, cs);
                    
                    for (i=0; i < num; i++) {
						if (lst[i].hasChild)
							sh_delete_selcell(&lst[i].d.child);
					}
                    num = 0;
                    NBK_free(cs);
                }
            }
        }
        else {
            if (t == '}' && *p == '{')
                lv++;
        }
        p++;
    }
    // when data is abort
    for (i=0; i < num; i++) {
		if (lst[i].hasChild)
			sh_delete_selcell(&lst[i].d.child);
	}
}

static int sh_split_names(char* names, char* str[])
{
    char* p;
    char* toofar;
    int i;
    
    // split names
    p = names;
    toofar = p + nbk_strlen(p);
    i = 0;
    while (p < toofar) {
        while (*p == ' ' && p < toofar)
            p++;
        if (p < toofar) {
            str[i++] = p;
            if (i == MAX_CLASS_NAME)
                break;
        }
        while (*p != ' ' && p < toofar)
            p++;
        if (p < toofar)
            *p++ = 0;
    }
    
    return i;
}

static void sh_join_names(char* names, int len)
{
    char* p;
    char* toofar;
    
    p = names;
    toofar = p + len;
    while (p < toofar) {
        if (*p == 0)
            *p = ' ';
        p++;
    }
}

static nbool sh_compare_sel_cell(NSelCell* c1, NSelCell* c2)
{
    if (c1->tid && (c1->tid != c2->tid))
        return N_FALSE;
    
    if (   (c1->type == SELT_CLASS || c1->type == SELT_TAG_CLASS)
        && (c2->type != SELT_CLASS) )
        return N_FALSE;
    
    if (   (c1->type == SELT_ID || c1->type == SELT_TAG_ID)
        && (c2->type != SELT_ID) )
        return N_FALSE;

    if (c1->type && (!c1->txt || !c2->txt))
        return N_FALSE;
    
    if (c1->type) {
        char* str[MAX_CLASS_NAME];
        int i, num, len;
        nbool found = N_FALSE;

        len = nbk_strlen(c2->txt);
        num = sh_split_names(c2->txt, str);
        for (i=0; i < num; i++) {
            if (nbk_strcmp(c1->txt, str[i]) == 0) {
                found = N_TRUE;
                break;
            }
        }
        if (num > 1)
            sh_join_names(c2->txt, len);
        
        return found;
    }
    
    return N_TRUE;
}

void sheet_getStyle(NSheet* sheet, NSelCell tagsc, NSelCell* des[], NCssStyle* style)
{
    NCssStyle* cs;
    NSelCell* ce;
    char* str[MAX_CLASS_NAME];
    int i, num, len;
    
    if (tagsc.tid == TAGID_TEXT)
        return;
    
    if (sheet->ts[0])
        css_addStyle(style, sheet->ts[0]);
    
    if (sheet->ts[tagsc.tid])
        css_addStyle(style, sheet->ts[tagsc.tid]);
    
    if (tagsc.type == SELT_CLASS) {
        len = nbk_strlen(tagsc.txt);
        num = sh_split_names(tagsc.txt, str);
        for (i=0; i < num; i++) {
            cs = (NCssStyle*)strDict_find(sheet->cs, str[i]);
            if (cs)
                css_addStyle(style, cs);
        }
        if (num > 1)
            sh_join_names(tagsc.txt, len);
    }
    else if (tagsc.type == SELT_ID) {
        len = nbk_strlen(tagsc.txt);
        num = sh_split_names(tagsc.txt, str);
        for (i=0; i < num; i++) {
            cs = (NCssStyle*)strDict_find(sheet->is, str[i]);
            if (cs)
                css_addStyle(style, cs);
        }
        if (num > 1)
            sh_join_names(tagsc.txt, len);
    }

    if (des == N_NULL)
        return;

    // check last selectors
    for (i=0; i < MAX_DES_SEL_USE; i++) {
        if (des[i] && sh_compare_sel_cell(des[i], &tagsc)) {
            if (des[i]->hasChild) {
                des[i] = des[i]->d.child;
            }
            else {
                css_addStyle(style, des[i]->d.style);
                des[i] = N_NULL;
            }
        }
    }
    
    for (i=0, num=0; i < MAX_DES_SEL_USE; i++)
        if (des[i] == N_NULL)
            num++;
    
    // match selectors
    ce = (NSelCell*)sll_first(sheet->ds);
    while (ce && num) {
        if (sh_compare_sel_cell(ce, &tagsc)) {
            if (ce->hasChild) {
                for (i=0; i < MAX_DES_SEL_USE; i++)
                    if (des[i] == N_NULL)
                        break;
                des[i] = ce->d.child;
                num--;
            }
            else {
                css_addStyle(style, ce->d.style);
            }
        }
        ce = (NSelCell*)sll_next(sheet->ds);
    }
}

void sheet_addData(NSheet* sheet, const char* style, int length)
{
    if (sheet->data == N_NULL)
        sheet->data = strBuf_create();
    
    strBuf_appendStr(sheet->data, style, length);
}

void sheet_parse(NSheet* sheet)
{
    char* data;
    int len;
    
    if (sheet->data == N_NULL)
        return;
    
    strBuf_joinAllStr(sheet->data);
    if (strBuf_getStr(sheet->data, &data, &len, N_TRUE))
        sheet_parseStyle(sheet, data, len);
    strBuf_delete(&sheet->data);
}
