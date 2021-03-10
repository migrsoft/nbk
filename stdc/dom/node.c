#include "node.h"
#include "xml_tags.h"
#include "xml_atts.h"
#include "view.h"
#include "page.h"
#include "document.h"
#include "attr.h"
#include "history.h"
#include "../inc/nbk_limit.h"
#include "../inc/nbk_callback.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_settings.h"
#include "../inc/nbk_ctlPainter.h"
#include "../css/color.h"
#include "../css/css_val.h"
#include "../css/css_value.h"
#include "../css/cssSelector.h"
#include "../css/css_helper.h"
#include "../dom/history.h"
#include "../render/imagePlayer.h"
#include "../render/renderNode.h"
#include "../render/renderInput.h"
#include "../render/renderSelect.h"
#include "../render/renderTextarea.h"
#include "../render/renderImage.h"
#include "../tools/slist.h"
#include "../tools/str.h"
#include "../tools/dump.h"
#include "../tools/unicode.h"
#include "../tools/memMgr.h"
#include "../loader/url.h"
#include "../loader/loader.h"

#define FONT_SIZE_SMALL     -2
#define FONT_SIZE_MEDIUM    -3
#define FONT_SIZE_LARGE     -4

#ifdef NBK_MEM_TEST
int node_memUsed(const NNode* node, int* txtLen, int* styLen)
{
    int size = 0;
    
    if (txtLen)
        *txtLen = 0;
    if (styLen)
        *styLen = 0;
    
    if (node) {
        size = sizeof(NNode);
        
        size += node->len * 2;
        if (txtLen)
            *txtLen = node->len * 2;
        
        if (node->ownAtts) {
            int sl;
            size += attr_memUsed(node->atts, &sl);
            if (styLen)
                *styLen = sl;
        }
    }

    return size;
}
#endif

NNode* node_create(NToken* token, int no)
{
	NNode* node;
#ifdef NBK_USE_MEMMGR
	node = (NNode*)node_alloc();
#else
	node = (NNode*)NBK_malloc0(sizeof(NNode));
#endif
	N_ASSERT(node);

#if DOM_TEST_INFO
	node->__no = no;
#endif
	node->id = token->id;
	node->space = token->space;

	if (token->id == TAGID_TEXT) {
		node->d.text = token->d.text;
		token->d.text = N_NULL;
		node->len = token->len;
	}

	node->atts = token->atts;
	token->atts = N_NULL;

	switch (node->id) {
	case TAGID_A:
		node->Click = nodeA_click;
		break;
	case TAGID_BUTTON:
	case TAGID_INPUT:
		node->Click = nodeInput_click;
		break;
	case TAGID_ANCHOR:
		node->Click = nodeAnchor_click;
		break;
	case TAGID_SELECT:
		node->Click = nodeSelect_click;
		break;
	case TAGID_TEXTAREA:
		node->Click = nodeTextarea_click;
		break;
	}

	return node;
}

void node_delete(NNode** node)
{
	NNode* t = *node;

	if (t->atts)
		attr_delete(&t->atts);

	if (t->d.text) {
		NBK_free(t->d.text);
		t->d.text = N_NULL;
	}

#ifdef NBK_USE_MEMMGR
	node_free(t);
#else
	NBK_free(t);
#endif
	*node = N_NULL;
}

static void gen_node_sel(NNode* node, NSelCell* cell)
{
    char* p;
    
    cell->tid = node->id;
    
    p = attr_getValueStr(node->atts, ATTID_CLASS);
    if (p) {
        cell->type = SELT_CLASS;
        cell->txt = p;
        return;
    }

    p = attr_getValueStr(node->atts, ATTID_ID);
    if (p) {
        cell->type = SELT_ID;
        cell->txt = p;
        return;
    }
}

/*
 * for all node
 */

typedef struct _NStyleStat {
    nid     nodeId;
    nbool    hasColor;
} NStyleStat;

static void comp_style_inherited(NStyle* style, const NCssStyle* cs, NStyleStat* stat)
{
    if (cs->hasTextAlign)
        style->text_align = cs->text_align;
    
    if (cs->hasTextDecor)
        style->underline = cs->text_decor;
    
    if (cs->text_indent.type)
        style->text_indent = cs->text_indent;
    
    if (cs->hasVertAlign)
        style->vert_align = cs->vert_align;

    if (cs->font_size.type) {
        if (cs->font_size.type == NECVT_INT)
            style->font_size = cs->font_size.d.i32;
        else if (cs->font_size.type == NECVT_VALUE) {
            if (cs->font_size.d.value == CSSVAL_SMALL)
                style->font_size = FONT_SIZE_SMALL;
            else if (cs->font_size.d.value == CSSVAL_LARGE)
                style->font_size = FONT_SIZE_LARGE;
            else
                style->font_size = FONT_SIZE_MEDIUM;
        }
    }
    
    if (cs->font_weight == CSS_FONT_BOLD)
        style->bold = 1;
    
    if (cs->line_height.type)
        style->line_height = cs->line_height;
    
    if (cs->hasColor) {
        style->color = cs->color;
        stat->hasColor = 1;
    }
    
    if (cs->hasOpacity)
        style->opacity = cs->opacity;
}

static void comp_style_non_inherit(NStyle* style, const NCssStyle* cs, NRenderNode* r)
{
    if (cs->flo != CSS_FLOAT_UNSET)
        style->flo = cs->flo;

    style->clr = cs->clr;
    style->z_index = cs->z_index;
    
    if (cs->display != CSS_DISPLAY_UNSET)
        style->display = cs->display;
    else if (r && renderNode_isBlockElement(r))
        style->display = CSS_DISPLAY_BLOCK;
    else
        style->display = CSS_DISPLAY_INLINE;
    
    if (cs->hasOverflow)
        style->overflow = cs->overflow;
    
    // for box model
    if (cs->hasMargin)
        css_boxAddSide(&style->margin, &cs->margin);
    if (cs->hasBorder)
        css_boxAddSide(&style->border, &cs->border);
    if (cs->hasPadding)
        css_boxAddSide(&style->padding, &cs->padding);
    
    // for color
    if (cs->hasBgcolor) {
        style->hasBgcolor = 1;
        style->background_color = cs->background_color;
    }
    if (cs->hasBrcolorL) {
        style->hasBrcolorL = 1;
        style->border_color_l = cs->border_color_l;
    }
    if (cs->hasBrcolorR) {
        style->hasBrcolorR = 1;
        style->border_color_r = cs->border_color_r;
    }
    if (cs->hasBrcolorT) {
        style->hasBrcolorT = 1;
        style->border_color_t = cs->border_color_t;
    }
    if (cs->hasBrcolorB) {
        style->hasBrcolorB = 1;
        style->border_color_b = cs->border_color_b;
    }

    // for background image
    if (cs->bgImage) {
        style->bgImage = cs->bgImage;
        style->bg_x = cs->bg_x; style->bg_x_t = cs->bg_x_t;
        style->bg_y = cs->bg_y; style->bg_y_t = cs->bg_y_t;
        style->bg_repeat = cs->bg_repeat;
    }

    if (cs->width.type)
        style->width = cs->width;
    if (cs->max_width.type)
        style->max_width = cs->max_width;
    if (cs->height.type)
        style->height = cs->height;
}

// for layout
void node_calcStyle(void* view, NNode* node, void* style)
{
    NView* v = (NView*)view;
    NPage* page = (NPage*)v->page;
    NNode* sta[MAX_TREE_DEPTH];
    NNode* n = node;
    NStyle* sty = (NStyle*)style;
    int16 top = -1;
    int16 i;
    NCssStyle* cs = N_NULL; // inline
    NCssStyle ncs; // sheet
    NSelCell sel;
    NSelCell* des[MAX_DES_SEL_USE];
    nbool hasColor = 0;
    nbool isLink = 0;
    NStyleStat stat;
    
    // initial state
    sty->bold = 0;
    sty->italic = 0;
    sty->text_align = 0;
    sty->color = colorBlack;
    
    // find path
    for (sta[++top] = n; n->parent; n = n->parent) {
        sta[++top] = n->parent;
    }
    
    NBK_memset(des, 0, sizeof(NSelCell*) * MAX_DES_SEL_USE);
    for (i=top; i >= 0; i--) { // from root to leaf
        
        n = sta[i];
        
        // get style from sheet
        NBK_memset(&ncs, 0, sizeof(NCssStyle));
        ncs.flo = CSS_FLOAT_UNSET;
        ncs.display = CSS_DISPLAY_UNSET;
        NBK_memset(&sel, 0, sizeof(NSelCell));
        gen_node_sel(n, &sel);
        sheet_getStyle(page->sheet, sel, des, &ncs);

        stat.hasColor = /*stat.hasBgimg =*/ 0;
        comp_style_inherited(sty, &ncs, &stat);
        if (stat.hasColor)
            hasColor = 1;

        // align style
        switch (n->id) {
        case TAGID_BODY:
        case TAGID_DIV:
        case TAGID_P:
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
        {
            nid type = attr_getType(n->atts, ATTID_ALIGN);
            if (type != N_INVALID_ID)
                sty->text_align = (uint8)type;
            break;
        }
        } // for align
        
        // font style
        switch (n->id) {
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
        case TAGID_B:
        case TAGID_STRONG:
            sty->bold = 1;
            break;
            
        case TAGID_BIG:
            sty->font_size += DEFAULT_FONT_ADD;
            break;
            
        case TAGID_SMALL:
            sty->font_size -= DEFAULT_FONT_ADD;
            break;
        } // for font
        
        // color style
        switch (n->id) {
        case TAGID_BODY:
        case TAGID_DIV:
        case TAGID_P:
        case TAGID_SPAN:
            if (i == 0) {
                NColor* color = attr_getColor(n->atts, ATTID_BGCOLOR);
                if (color) {
                    sty->hasBgcolor = 1;
                    sty->background_color = *color;
                }
            }
            break;
            
        case TAGID_A:
        case TAGID_ANCHOR:
            isLink = 1;
            if (!hasColor)
                sty->color = colorBlue;
            break;
            
        case TAGID_FONT:
        {
            NColor* color = attr_getColor(n->atts, ATTID_COLOR);
            if (color) {
                hasColor = 1;
                sty->color = *color;
            }
            break;
        }
        } // for color

        // get inline style
        cs = attr_getStyle(n->atts);
        if (cs) {
            stat.hasColor = /*stat.hasBgimg =*/ 0;
            comp_style_inherited(sty, cs, &stat);
            if (stat.hasColor)
                hasColor = 1;
        }
    }
    
    if (sty->font_size > MAX_FONT_SIZE)
        sty->font_size = MAX_FONT_SIZE;
    
    // from style sheet
    comp_style_non_inherit(sty, &ncs, (NRenderNode*)n->render);
    if (cs) // inline style
        comp_style_non_inherit(sty, cs, (NRenderNode*)n->render);
    
    if (   page->settings
        && page->settings->selfAdaptionLayout
        && page->settings->mainFontSize
        && n->id != TAGID_IMG )
    {
        // self-adaption mode
        // 忽略样式指定的字号、宽、高信息
        coord da;
        sty->width.type = NECVT_NONE;
        sty->max_width.type = NECVT_NONE;
        sty->height.type = NECVT_NONE;
        if (sty->line_height.type == NECVT_INT) {
            da = sty->line_height.d.i32 - sty->font_size;
            if (da < 0) da = 0;
            sty->line_height.d.i32 = page->settings->mainFontSize + da;
        }
        sty->font_size = page->settings->mainFontSize;
    }
    else if (sty->font_size <= 0) {
        int16 fsize;
        if (page->settings && page->settings->mainFontSize)
            fsize = page->settings->mainFontSize;
        else
            fsize = DEFAULT_FONT_SIZE;

        if (sty->font_size == FONT_SIZE_SMALL)
            sty->font_size = fsize - DEFAULT_FONT_ADD;
        else if (sty->font_size == FONT_SIZE_LARGE)
            sty->font_size = fsize + DEFAULT_FONT_ADD;
        else
            sty->font_size = fsize;
    }
    
    // apply night theme
    if (page->settings && page->settings->night) {
        sty->color = (isLink) ? page->settings->night->link : page->settings->night->text;
        sty->background_color = \
        sty->border_color_l = \
        sty->border_color_t = \
        sty->border_color_r = \
        sty->border_color_b = page->settings->night->background;
    }
}

static int get_upload_file_worker(NSList* lst, int* num, NFileUpload** files)
{
    NNode* node = (NNode*)sll_first(lst);
    int i = 0;
    NFileUpload* p = N_NULL;

    if (num)
        p = (NFileUpload*)NBK_malloc0(sizeof(NFileUpload) * (*num + 1));
    
    while (node) {
        if (node->id == TAGID_INPUT) {
            NRenderInput* ri = (NRenderInput*)node->render;
            if (ri && ri->type == NEINPUT_FILE && ri->text) {
                if (p) {
                    p[i].name = attr_getValueStr(node->atts, ATTID_NAME);
                    p[i].path = ri->text;
                    //p[i].nidx = attr_getValueInt32(node->atts, ATTID_TC_NIDX);
                }
                i++;
            }
        }
        
        node = (NNode*)sll_next(lst);
    }
    
    if (files)
        *files = p;
    
    return i;
}

static NFileUpload* get_upload_files(NSList* lst)
{
    NFileUpload* files = N_NULL;
    int num;
    
    num = get_upload_file_worker(lst, N_NULL, N_NULL);
    if (num)
        get_upload_file_worker(lst, &num, &files);
    
    return files;
}

static int form_urlencoded_worker(NSList* fields, NNode* submit, char** body)
{
    int total = 0, l;
    NNode *n, *m;
    char *name, *value;
    char *p, *q;
    
    p = (body) ? *body : N_NULL;

    //if (p) {
    //    dump_char(g_dp, "===================", -1);
    //    dump_return(g_dp);
    //}
    
    n = (NNode*)sll_first(fields);
    while (n) {
        
        if (n->id == TAGID_INPUT) {
            NRenderInput* ri = (NRenderInput*)n->render;
            if (ri->type != NEINPUT_SUBMIT || (ri->type == NEINPUT_SUBMIT && n == submit)) {
                total += 2;
                
                name = attr_getValueStr(n->atts, ATTID_NAME);
                if (name) {
                    l = nbk_strlen(name);
                    total += l;

                    q = p;
                    if (p) {
                        if (p > *body)
                            *p++ = '&';
                        nbk_strcpy(p, name);
                        p += l;
                        *p++ = '=';
                    }
                    
                    l = 0;
                    value = N_NULL;
                    if (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD) {
                        l = renderInput_getEditTextLen(ri) * 3;
                    }
                    else if (ri->type == NEINPUT_CHECKBOX || ri->type == NEINPUT_RADIO) {
                        if (ri->checked) {
                            value = attr_getValueStr(n->atts, ATTID_VALUE);
                            l = (value) ? nbk_strlen(value) * 3 : 0;
                        }
                        else
                            p = q;
                    }
                    else {
                        value = attr_getValueStr(n->atts, ATTID_VALUE);
                        l = (value) ? nbk_strlen(value) * 3 : 0;
                    }
                    total += l;

                    //if (p)
                    //    dump_char(g_dp, q, p - q);
                    
                    if (p && l) {
                        nbool del = (value) ? 0 : 1;
                        q = p;
                        if (value == N_NULL)
                            value = renderInput_getEditText(ri);
                        p = url_hex_to_str(p, value);
                        //dump_char(g_dp, q, p - q);
                        if (del)
                            NBK_free(value);
                    }

                    //if (p)
                    //    dump_return(g_dp);
                }
            }
        }
        else if (n->id == TAGID_SELECT) {
            total += 2;
            
            name = attr_getValueStr(n->atts, ATTID_NAME);
            l = nbk_strlen(name);
            total += l;
            if (p) {
                if (p > *body)
                    *p++ = '&';
                nbk_strcpy(p, name);
                p += l;
                *p++ = '=';
            }
            
            l = 0;
            m = n->child;
            while (m) {
                if (m->id == TAGID_OPTION) {
                    if (attr_getValueBool(m->atts, ATTID_SELECTED_P)) {
                        value = attr_getValueStr(m->atts, ATTID_VALUE);
                        l = (value) ? nbk_strlen(value) : 0;
                        total += l;
                        
                        if (p && l) {
                            p = url_hex_to_str(p, value);
                        }
                        break;
                    }
                }
                m = m->next;
            }
        }
        else if (n->id == TAGID_TEXTAREA) {
            NRenderTextarea* r = (NRenderTextarea*)n->render;
            total += 2;
            
            name = attr_getValueStr(n->atts, ATTID_NAME);
            l = nbk_strlen(name);
            total += l;
            if (p) {
                if (p > *body)
                    *p++ = '&';
                nbk_strcpy(p, name);
                p += l;
                *p++ = '=';
            }
            
            l = renderTextarea_getEditTextLen(r) * 3;
            total += l;
            if (p && l) {
                value = renderTextarea_getEditText(r);
                p = url_hex_to_str(p, value);
                NBK_free(value);
            }
        }
        
        n = (NNode*)sll_next(fields);
    }
    
    if (body) {
        *p++ = 0;
        *body = p;
    }
    return total;
}

static char* form_urlencoded(char* query, NSList* eles, NNode* submit)
{
    if (query) {
        form_urlencoded_worker(eles, submit, &query);
        return query;
    }
    else {
        int need = form_urlencoded_worker(eles, submit, N_NULL);
        char* body = (char*)NBK_malloc(need);
        char* p = body;
        form_urlencoded_worker(eles, submit, &p);
        return body;
    }
}

static int form_multipart_worker(NSList* fields, NNode* submit, char** body)
{
    int total, s;
    NNode* n;
    char *name, *value;
    char *p = (body) ? *body : N_NULL;
    
    total = 0;
    n = (NNode*)sll_first(fields);
    while (n) {
        if (n->id == TAGID_INPUT) {
            NRenderInput* ri = (NRenderInput*)n->render;
            if (   (ri->type != NEINPUT_SUBMIT && ri->type != NEINPUT_FILE)
                || (ri->type == NEINPUT_SUBMIT && n == submit)) {
                name = attr_getValueStr(n->atts, ATTID_NAME);
                if (name) {
                    nbool del = N_FALSE;
                    value = N_NULL;
                    if (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD) {
                        del = N_TRUE;
                        value = renderInput_getEditText(ri);
                    }
                    else if (ri->type == NEINPUT_CHECKBOX || ri->type == NEINPUT_RADIO) {
                        if (ri->checked)
                            value = attr_getValueStr(n->atts, ATTID_VALUE);
                        else
                            name = N_NULL;
                    }
                    else {
                        value = attr_getValueStr(n->atts, ATTID_VALUE);
                    }

                    if (name) {
                        s = multipart_field(name, value, p);
                        total += s;
                        if (p)
                            p += s;
                        if (del)
                            NBK_free(value);
                    }
                }
            }
        }
        else if (n->id == TAGID_SELECT) {
            name = attr_getValueStr(n->atts, ATTID_NAME);
            if (name) {
                NNode* m = n->child;
                while (m) {
                    if (m->id == TAGID_OPTION) {
                        if (attr_getValueBool(m->atts, ATTID_SELECTED_P)) {
                            value = attr_getValueStr(m->atts, ATTID_VALUE);
                            s = multipart_field(name, value, p);
                            total += s;
                            if (p)
                                p += s;
                            break;
                        }
                    }
                    m = m->next;
                }
            }
        }
        else if (n->id == TAGID_TEXTAREA) {
            NRenderTextarea* r = (NRenderTextarea*)n->render;
            name = attr_getValueStr(n->atts, ATTID_NAME);
            if (name) {
                value = renderTextarea_getEditText(r);
                s = multipart_field(name, value, p);
                total += s;
                if (p)
                    p += s;
                NBK_free(value);
            }
        }
        
        n = (NNode*)sll_next(fields);
    }
    
    return total;
}

static char* form_multipart(NSList* eles, NNode* submit, NFileUpload** files)
{
    char* body = N_NULL;
    int size = form_multipart_worker(eles, submit, N_NULL);
    *files = N_NULL;
    if (size > 0) {
        size += multipart_end(N_NULL);
        body = (char*)NBK_malloc(size + 32);
        size = form_multipart_worker(eles, submit, &body);
        *files = get_upload_files(eles);
        if (*files == N_NULL)
            multipart_end(body + size);
    }
    return body;
}

static void form_submit_by_get(NDocument* doc, char* action, NSList* eles, NNode* submit)
{
    char* url = url_parse(doc_getUrl(doc), action);
    char* p = url + nbk_strlen(url);
    nbool hasQuery = url_hasQuery(url);

    if (hasQuery)
        *p++ = '&';
    else
        *p++ = '?';
    p = form_urlencoded(p, eles, submit);
    if (!hasQuery && *(p-1) == '?')
        *--p = 0;
    
    doc_doGet(doc, url, N_TRUE, N_FALSE);
    
    NBK_free(url);
}

static void form_submit_by_post(NDocument* doc, char* action, nid enc, NSList* eles, NNode* submit)
{
    NPage* page = (NPage*)doc->page;
    char* body = N_NULL;
    NFileUpload* files = N_NULL;
    
    if (enc == NEENCT_URLENCODED)
        body = form_urlencoded(N_NULL, eles, submit);
    else if (enc == NEENCT_MULTIPART)
        body = form_multipart(eles, submit, &files);
    
//    if (body) {
//        int len = nbk_strlen(body);
//        dump_char(page, body, len);
//        dump_return(page);
//        dump_flush(page);
//    }
    
    if (body)
        doc_doPost(doc, action, enc, body, files);
    else if (files)
        NBK_free(files);
}

static void get_all_controls(NDocument* doc, NNode* root, NSList* lst)
{
    NNode* n = root;
    NNode* sta[MAX_TREE_DEPTH];
    int16 lv = -1;
    nbool ignore = N_FALSE;
    
    while (n) {
        
        if (!ignore) {
            if (   n->id == TAGID_INPUT
                /*|| n->id == TAGID_FRAME*/
                || n->id == TAGID_TEXTAREA
                || n->id == TAGID_POSTFIELD ) {
                sll_append(lst, n);
            }
        }
        
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
}

static void form_submit(NDocument* doc, NNode* submit)
{
    NSList* lst;
    NNode* n = submit;
    char* action;
    char* method;
    char* enctype;
    nbool byGet = N_TRUE;
    nid enc = NEENCT_URLENCODED;
    nid tags[] = {TAGID_INPUT, TAGID_TEXTAREA, TAGID_SELECT, 0};

    // find form node
    while (n && n->id != TAGID_FORM)
        n = n->parent;
    if (n == N_NULL)
        return;
    
    if ((action = attr_getValueStr(n->atts, ATTID_ACTION)) == N_NULL)
        return;

    if ((method = attr_getValueStr(n->atts, ATTID_METHOD)) != N_NULL) {
        if (nbk_strcmp(method, "post") == 0)
            byGet = N_FALSE;
    }
    
    if ((enctype = attr_getValueStr(n->atts, ATTID_ENCTYPE)) != N_NULL) {
        if (nbk_strcmp(enctype, "multipart/form-data") == 0)
            enc = NEENCT_MULTIPART;
    }
    
    lst = node_getListByTag(n, tags);
    
    if (byGet)
        form_submit_by_get(doc, action, lst, submit);
    else
        form_submit_by_post(doc, action, enc, lst, submit);
    
    sll_delete(&lst);
}

//static void form_submit_dx(NDocument* doc, NNode* submit)
//{
//    NPage* page = (NPage*)doc->page;
//    NSList* lst;
//    NNode* n = submit;
//    nid tags[] = {TAGID_INPUT, TAGID_TEXTAREA, TAGID_SELECT, 0};
//    char* action;
//    char* charset;
//    char* method;
//    char* enctype;
//    NUpCmd* upcmd;
//    
//    while (n && n->id != TAGID_FORM)
//        n = n->parent;
//    if (n == N_NULL)
//        return;
//    
//    action = attr_getValueStr(n->atts, ATTID_ACTION);
//    if (action == N_NULL)
//        return;
//    
//    lst = node_getListByTag(n, tags);
//
//    charset = attr_getValueStr(n->atts, ATTID_CHARSET);
//    method = attr_getValueStr(n->atts, ATTID_METHOD);
//    enctype = attr_getValueStr(n->atts, ATTID_ENCTYPE);
//    
//    upcmd = upcmd_create(N_NULL);
//    upcmd_formSubmit(upcmd, doc_getUrl(doc), action, charset, method, enctype);
//    upcmd_formFields(upcmd, lst);
//    
//    n = (NNode*)sll_first(lst);
//    while (n) {
//        if (n->id == TAGID_TEXTAREA) {
//            NRenderTextarea* r = (NRenderTextarea*)n->render;
//            char* name = attr_getValueStr(n->atts, ATTID_NAME);
//            int len = renderTextarea_getEditTextLen(r);
//            if (name && len) {
//                char* value = renderTextarea_getEditText(r);
//                if (value) {
//                    upcmd_formTextarea(upcmd, name, value, len);
//                    NBK_free(value);
//                }
//            }
//        }
//        
//        n = (NNode*)sll_next(lst);
//    }
//    
//    sll_delete(&lst);
//    
//    doc_doIa(doc, upcmd, N_NULL);
//}

static void form_go(NDocument* doc, NNode* go)
{
    NSList* lst;
    NNode* n = go;

    while (n && n->id != TAGID_FORM)
        n = n->parent;
    if (n == N_NULL)
        return;
    
    lst = sll_create();
    get_all_controls(doc, n, lst);
    n = (NNode*)sll_first(lst);
    while (n) {
        if (n->id == TAGID_INPUT) {
            NRenderInput* ri = (NRenderInput*)n->render;
            if (ri->type == NEINPUT_TEXT) {
                char* url = renderInput_getEditText(ri);
                if (url) {
                    const char* http = "http://";
                    const char* file = "file://";
                    if (nbk_strncmp(url, http, 7) && nbk_strncmp(url, file, 7)) {
                        char* u = (char*)NBK_malloc(nbk_strlen(url) + 8);
                        char* p = u;
                        nbk_strcpy(p, http);
                        p += 7;
                        nbk_strcpy(p, url);
                        NBK_free(url);
                        url = u;
                    }
                    doc_doGet(doc, url, N_FALSE, N_FALSE);
                    NBK_free(url);
                }
                break;
            }
        }
        n = (NNode*)sll_next(lst);
    }
    sll_delete(&lst);
}

static void form_radio_selected(NDocument* doc, NNode* radio)
{
    NSList* lst;
    NNode* n = radio;
    char* group = N_NULL;
    
    // find form node
    while (n && n->id != TAGID_FORM && n->id != TAGID_BODY) {
        n = n->parent;
    }
    if (n == N_NULL)
        return;
    
    group = attr_getValueStr(radio->atts, ATTID_NAME);
    if (group == N_NULL)
        return;
    
    lst = sll_create();
    get_all_controls(doc, n, lst);
    
    n = (NNode*)sll_first(lst);
    while (n) {
        if (n->id == TAGID_INPUT) {
            NRenderInput* ri = (NRenderInput*)n->render;
            if (ri->type == NEINPUT_RADIO) {
                char* name = N_NULL;
                if ((name = attr_getValueStr(n->atts, ATTID_NAME))) {
                    if (nbk_strcmp(name, group) == 0)
                        ri->checked = (n == radio) ? N_TRUE : N_FALSE;
                }
            }
        }
        
        n = (NNode*)sll_next(lst);
    }
    
    sll_delete(&lst);
}

static int anchor_urlencoded_worker(NSList* fields, NSList* all, char** body)
{
    int total = 0;
    int l;
    NNode *n, *m;
    char *vn, *name, *value, t;
    char *p, *q;
    
    p = (body) ? *body : N_NULL;
    
    n = (NNode*)sll_first(fields);
    while (n) {
        if (n->id == TAGID_POSTFIELD
            && (name = attr_getValueStr(n->atts, ATTID_NAME))
            && (vn = attr_getValueStr(n->atts, ATTID_VALUE)) ) {

            total += 2; // '&' + '='
            l = nbk_strlen(name);
            total += l;
            
            if (p) {
                if (p > *body)
                    *p++ = '&';
                nbk_strcpy(p, name);
                p += l;
                *p++ = '=';
            }

            if (*vn == '$') {
                q = vn;
                while (*q == '$' || *q == '(')
                    q++;
                vn = q;
                while (*q && *q != ')')
                    q++;
                t = *q;
                *q = 0;
                
                m = (NNode*)sll_first(all);
                while (m) {
                    if (m->id == TAGID_INPUT) {
                        NRenderInput* ri = (NRenderInput*)m->render;
                        if (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD) {
                            name = attr_getValueStr(m->atts, ATTID_NAME);
                            if (name && nbk_strcmp(name, vn) == 0) {
                                l = renderInput_getEditTextLen(ri) * 3;
                                total += l;
                                
                                if (p) {
                                    value = renderInput_getEditText(ri);
                                    if (value) {
                                        p = url_hex_to_str(p, value);
                                        NBK_free(value);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    else if (m->id == TAGID_SELECT) {
                        name = attr_getValueStr(m->atts, ATTID_NAME);
                        if (name && nbk_strcmp(name, vn) == 0) {
                            NNode* op = m->child;
                            while (op) {
                                if (   op->id == TAGID_OPTION
                                    && attr_getValueBool(op->atts, ATTID_SELECTED_P)) {
                                    value = attr_getValueStr(op->atts, ATTID_VALUE);
                                    if (value) {
                                        l = nbk_strlen(value);
                                        total += l;
                                        
                                        if (p) {
                                            nbk_strcpy(p, value);
                                            p += l;
                                        }
                                    }
                                    break;
                                }
                                op = op->next;
                            }
                            break;
                        }
                    }
                    m = (NNode*)sll_next(all);
                }
                
                *q = t; // restore
            }
            else {
                l = nbk_strlen(vn) * 3;
                total += l;
                
                if (p)
                    p = url_hex_to_str(p, vn);
            }
        }
        n = (NNode*)sll_next(fields);
    }
    
    if (body) {
        *p++ = 0;
        *body = p;
    }
    return total;
}

static char* anchor_urlencoded(char* query, NSList* fields, NSList* all)
{
    if (query) {
        anchor_urlencoded_worker(fields, all, &query);
        return query;
    }
    else {
        int need = anchor_urlencoded_worker(fields, all, N_NULL);
        char* body = (char*)NBK_malloc(need);
        char* p = body;
        anchor_urlencoded_worker(fields, all, &p);
        return body;
    }
}

static void anchor_sumbit_by_get(NDocument* doc, char* href, NSList* fields, NSList* all)
{
    char* url = url_parse(doc_getUrl(doc), href);
    char* p = url + nbk_strlen(url);
    nbool hasQuery = url_hasQuery(url);

    if (hasQuery)
        *p++ = '&';
    else
        *p++ = '?';
    p = anchor_urlencoded(p, fields, all);
    if (!hasQuery && *(p-1) == '?')
        *--p = 0;
    
    doc_doGet(doc, url, N_TRUE, N_FALSE);
    
    NBK_free(url);
}

static void anchor_submit_by_post(NDocument* doc, char* href, nid enc, NSList* fields, NSList* all)
{
    char* body = anchor_urlencoded(N_NULL, fields, all);
    doc_doPost(doc, href, enc, body, N_NULL);
}

#define NTASK_NEDISP_NUM 2

typedef struct _NTaskNedisplay {
    char*           match;
    NRenderNode*    render[NTASK_NEDISP_NUM];
    int             num;
} NTaskNedisplay;

static int task_ne_dispaly(NNode* node, void* user, nbool* ignore)
{
    NTaskNedisplay* task = (NTaskNedisplay*)user;
    
    if (node->id == TAGID_FRAME) {
        *ignore = N_TRUE;
    }
    else if (node->id == TAGID_DIV) {
        char* name = attr_getValueStr(node->atts, ATTID_NAME);
        if (name && !nbk_strcmp(name, task->match)) {
            task->render[task->num++] = (NRenderNode*)node->render;
            if (task->num == NTASK_NEDISP_NUM)
                return 1;
        }
    }
    
    return 0;
}

void nodeA_click(NNode* node, void* doc)
{
	NDocument* d = (NDocument*)doc;
	char* url = attr_getValueStr(node->atts, ATTID_HREF);

	if (url == N_NULL)
		url = attr_getValueStr(node->atts, ATTID_SRC); // translate from frame
	if (url == N_NULL)
		url = doc_getUrl(d);

	if (url) {
		if (*url == '#')
			doc_jumpTo(d, url + 1);
		else if (   nbk_strfind(url, "javascript:history.back()") != -1
				 || nbk_strfind(url, "javascript:history.go(-1)") != -1 )
			doc_doBack(d);
		else
			doc_doGet(d, url, N_FALSE, N_FALSE);
	}
}

void nodeAttachment_click(NNode* node, void* doc)
{
    NPage* page = (NPage*)((NDocument*)doc)->page;
    char* src = attr_getValueStr(node->atts, ATTID_SRC);
    NBK_Event_DownloadFile evt;

    if (src) {
        evt.size = attr_getCoord(node->atts, ATTID_SIZE);
        evt.url = src; 
        evt.cookie = N_NULL;
        evt.fileName = attr_getValueStr(node->atts, ATTID_TC_FILENAME);
        nbk_cb_call(NBK_EVENT_DOWNLOAD_FILE, &page->cbEventNotify, &evt);
    }
}

static void repos_editor(NRenderNode* rn, NPage* page)
{
    nbool repos = N_FALSE;
    NBK_Event_RePosition evt;
    NRect va;
    int x, y, w, h;
    float zoom = history_getZoom((NHistory*)page->history);
    
    NBK_helper_getViewableRect(page->platform, &va);
    
    if (rn->type == RNT_INPUT) {
        NRenderInput* ri = (NRenderInput*)rn;
        if (!renderInput_isEditing(ri)) {
            x = (coord)float_imul(rn->r.l, zoom);
            y = (coord)float_imul(rn->r.t, zoom);
            repos = N_TRUE;
        }
    }
    else if (rn->type == RNT_TEXTAREA) {
        NRenderTextarea* rt = (NRenderTextarea*)rn;
        if (!renderTextarea_isEditing(rt)) {
            x = (coord)float_imul(rn->r.l, zoom);
            y = (coord)float_imul(rn->r.t, zoom);
            repos = N_TRUE;
        }
    }
    
    if (repos) {
        coord da;
        NPoint pos;
        
        da = 0;
        
        w = rect_getWidth(&rn->r);
        h = rect_getHeight(&rn->r);

        if (w > rect_getWidth(&va))
            w = rect_getWidth(&va);
        w = w - DEFAULT_EDITOR_HORI_SPACEING;
        if (h > rect_getHeight(&va))
            h = rect_getHeight(&va);
        h = h - DEFAULT_EDITOR_VERTICAL_SPACEING;
                
        if (rect_getWidth(&va) > w)
            da = (rect_getWidth(&va) - w) / 2;

        pos.x = x - da;
        pos.y = y - MAX_EDITOR_VERTICAL_OFFSET;

        view_normalizePoint(page->view, &pos, va, zoom);
        
        evt.x = pos.x;
        evt.y = pos.y;
        evt.zoom = zoom;
        nbk_cb_call(NBK_EVENT_REPOSITION, &page->cbEventNotify, &evt);
    }
}

void nodeInput_click(NNode* node, void* doc)
{
    NDocument* d = (NDocument*)doc;
    NPage* page = d->page;
    NRenderInput* ri = (NRenderInput*)node->render;
    
    switch (ri->type) {
    case NEINPUT_TEXT:
    case NEINPUT_PASSWORD:
        renderInput_edit(ri, doc);
        nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
        break;
        
    case NEINPUT_IMAGE:
    case NEINPUT_SUBMIT:
        form_submit(page->doc, node);
        break;
        
    case NEINPUT_BUTTON:
        break;
        
    case NEINPUT_CHECKBOX:
        ri->checked = !ri->checked;
        nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
        break;
        
    case NEINPUT_RADIO:
        form_radio_selected(page->doc, node);
        nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
        break;
        
    case NEINPUT_FILE:
        {
            char* path = N_NULL;
            if (NBK_browseFile(page->platform, ri->text, &path)) {
                renderInput_setText(ri, path);
            }
            nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
        }
        break;
        
        // NBK 自定义控件
    case NEINPUT_GO:
        form_go(page->doc, node);
        break;
    }
}

void nodeAnchor_click(NNode* node, void* doc)
{
    NPage* page = ((NDocument*)doc)->page;
    NNode* go;
    char* href;
    char* method;
    nbool byGet = N_TRUE;
    NSList* fields;
    NSList* all;
    nid tags[] = {TAGID_INPUT, TAGID_TEXTAREA, TAGID_POSTFIELD, TAGID_SELECT, 0};
    
    go = node_getByTag(node, TAGID_GO);
    if (!go)
        return;
    
    href = attr_getValueStr(go->atts, ATTID_HREF);
    if (href == N_NULL)
        return;
    
    method = attr_getValueStr(go->atts, ATTID_METHOD);
    if (method && nbk_strcmp(method, "post") == 0)
        byGet = N_FALSE;
    
    fields = node_getListByTag(go, tags);
    all = node_getListByTag(page->doc->root, tags);
    
    if (byGet)
        anchor_sumbit_by_get(page->doc, href, fields, all);
    else
        anchor_submit_by_post(page->doc, href, NEENCT_URLENCODED, fields, all);
    
    sll_delete(&fields);
    sll_delete(&all);
}

void nodeSelect_click(NNode* node, void* doc)
{
    NPage* page = ((NDocument*)doc)->page;
    NRenderSelect* rs =(NRenderSelect*) node->render;
    if (renderSelect_popup(rs)) {
        nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
    }
}

void nodeTextarea_click(NNode* node, void* doc)
{
    NDocument* d = (NDocument*)doc;
    NPage* page = (NPage*)d->page;
    NRenderTextarea* rt = (NRenderTextarea*)node->render;
    renderTextarea_edit(rt, doc);
    nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
}

void nodeDiv_click(NNode* node, void* doc)
{
}

void nodeImg_click(NNode* node, void* doc)
{
}

NNode* node_getByTag(NNode* root, const nid tagId)
{
    NNode* n = root;
    nbool ignore = N_FALSE;
    NNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    
    while (n) {
        if (!ignore) {
            if (n->id == tagId)
                return n;
        }
        
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    return N_NULL;
}

NSList* node_getListByTag(NNode* root, const nid* tags)
{
    NNode* n = root;
    nbool ignore = N_FALSE;
    NNode* sta[MAX_TREE_DEPTH];
    int16 lv = -1;
    int i;
    NSList* lst = sll_create();
    
    while (n) {
        
        if (n != root && n->id == TAGID_HTML) { // don't search sub page
            char* id = attr_getValueStr(n->atts, ATTID_ID);
            if (id && !nbk_strcmp(id, "ff-full"))
                ignore = 1;
        }
        
        if (!ignore) {
            for (i=0; tags[i]; i++) {
                if (n->id == tags[i])
                    sll_append(lst, n);
            }
        }
        
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    return lst;
}

NNode* node_getParentById(NNode* node, nid id)
{
    NNode* n = node;
    while (n && n->id != id)
        n = n->parent;
    if (n && n->id == id)
        return n;
    else
        return N_NULL;
}

void node_traverse_depth(NNode* root,
                         DOM_TRAVERSE_START_CB startCb,
                         DOM_TRAVERSE_CB endCb,
                         void* user)
{
    NNode* n = root;
    nbool ignore = N_FALSE;
    NNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    
    while (n) {
        
        if (!ignore) {
            if (startCb && startCb(n, user, &ignore))
                break;
        }
        
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            if (endCb && !ignore && n->child == N_NULL && !node_is_single(n->id)) {
                if (endCb(n, user))
                    break;
            }
            
            ignore = N_FALSE;
            
            if (n == root)
                break;
            else if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                if (endCb && endCb(n, user))
                    break;
                ignore = N_TRUE;
            }
        }
    }
}

nbool node_is_single(nid id)
{
    nid i = (id > TAGID_CLOSETAG) ? id - TAGID_CLOSETAG : id;
    switch (i) {
    case TAGID_BASE:
    case TAGID_META:
    case TAGID_INPUT:
    case TAGID_BR:
    case TAGID_IMG:
    case TAGID_HR:
    case TAGID_TEXT:
    case TAGID_TIMER:
        return N_TRUE;
    default:
        return N_FALSE;
    }
}

int node_getEditLength(NNode* node)
{
    if (node && (node->id == TAGID_INPUT || node->id == TAGID_TEXTAREA))
        return renderNode_getEditLength((NRenderNode*)node->render);
    else
        return 0;
}

int node_getEditMaxLength(NNode* node)
{
    if (node && (node->id == TAGID_INPUT || node->id == TAGID_TEXTAREA))
        return renderNode_getEditMaxLength((NRenderNode*)node->render);
    else
        return 0;
}

void node_cancelEditing(NNode* node, void* page)
{
    NPage* p = (NPage*)page;
    
    if (node == N_NULL)
        return;
    
    if (node->id == TAGID_INPUT) {
        NRenderInput* ri = (NRenderInput*)node->render;
        if (ri && ri->inEdit && (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD)) {
            ri->inEdit = 0;
            NBK_fep_disable(p->platform);
            editBox_maskAll(ri->editor);
        }
    }
    else if (node->id == TAGID_TEXTAREA) {
        NRenderTextarea* rt = (NRenderTextarea*)node->render;
        if (rt && rt->inEdit) {
            rt->inEdit = 0;
            NBK_fep_disable(p->platform);
        }
    }
    else if (node->id == TAGID_SELECT) {
        NRenderSelect* rs = (NRenderSelect*)node->render;
        if (rs)
            renderSelect_dismiss(rs);
    }
}
