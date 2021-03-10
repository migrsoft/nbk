/*
 * view.c
 *
 *  Created on: 2010-12-29
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "../inc/nbk_limit.h"
#include "../inc/nbk_callback.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_gdi.h"
#include "../inc/nbk_settings.h"
#include "../css/color.h"
#include "../css/css_val.h"
#include "../css/cssSelector.h"
#include "../css/css_helper.h"
#include "../tools/dump.h"
#include "../tools/str.h"
#include "../render/renderNode.h"
#include "../render/renderBlock.h"
#include "../render/renderBlank.h"
#include "../render/renderImage.h"
#include "../render/renderInput.h"
#include "../render/renderText.h"
#include "../render/renderTextPiece.h"
#include "../render/renderSelect.h"
#include "../render/renderObject.h"
#include "../tools/dump.h"
#include "../tools/str.h"
#include "view.h"
#include "node.h"
#include "attr.h"
#include "page.h"
#include "document.h"
#include "history.h"
#include "xml_tags.h"
#include "xml_atts.h"

#define DUMP_FUNCTIONS      1
#define DUMP_RENDER_TREE    0

#define TEST_PAINT_TIME     0

#define PAINT_TIME_SAMPLING 50

#ifdef NBK_MEM_TEST
int view_memUsed(const NView* view)
{
    int size = 0;
    if (view)
        size = sizeof(NView);
    return size;
}
#endif

/***** Z-Index Matrix *****/

static NZidxMatrix* zidxMatrix_create(void)
{
    NZidxMatrix* zmx = (NZidxMatrix*)NBK_malloc0(sizeof(NZidxMatrix));
    zmx->panels = (NZidxPanel*)NBK_malloc0(sizeof(NZidxPanel) * ZIDX_PANEL);
    zmx->max = ZIDX_PANEL;
    return zmx;
}

static void zidxMatrix_delete(NZidxMatrix** matrix)
{
    NZidxMatrix* zmx = *matrix;
    NZidxPanel* pan;
    int i;
    for (i=0; i < zmx->use; i++) {
        pan = &zmx->panels[i];
        NBK_free(pan->cells);
    }
    NBK_free(zmx->panels);
    NBK_free(zmx);
    *matrix = N_NULL;
}

static void zidxMatrix_reset(NZidxMatrix* zmx)
{
    int i;
    zmx->ready = N_FALSE;
    zmx->use = 0;
    for (i=0; i < zmx->max; i++) {
        zmx->panels[i].z = 0;
        zmx->panels[i].use = 0;
    }
}

static void zidxMatrix_cellAppend(NZidxMatrix* zmx, int i, int32 z, NRenderNode* rn, NRect rect)
{
    nbool normal = (   rect_getWidth(&rn->r) == rect_getWidth(&rect)
                   && rect_getHeight(&rn->r) == rect_getHeight(&rect) ) ? N_TRUE : N_FALSE;
    
    zmx->panels[i].z = z;
    
    if (zmx->panels[i].max == 0) {
        zmx->panels[i].use = 1;
        zmx->panels[i].max = ZIDX_CELL_IN_PANEL;
        zmx->panels[i].cells = (NZidxCell*)NBK_malloc0(sizeof(NZidxCell) * ZIDX_CELL_IN_PANEL);
        zmx->panels[i].cells[0].rn = rn;
        zmx->panels[i].cells[0].rect = rect;
        zmx->panels[i].cells[0].normal = normal;
    }
    else {
        if (zmx->panels[i].use == zmx->panels[i].max) {
            zmx->panels[i].max += ZIDX_CELL_IN_PANEL;
            zmx->panels[i].cells = (NZidxCell*)NBK_realloc(
                zmx->panels[i].cells, sizeof(NZidxCell) * zmx->panels[i].max);
        }
        
        zmx->panels[i].cells[zmx->panels[i].use].rn = rn;
        zmx->panels[i].cells[zmx->panels[i].use].rect = rect;
        zmx->panels[i].cells[zmx->panels[i].use].normal = normal;
        zmx->panels[i].use++;
    }
}

static void zidxMatrix_append(NZidxMatrix* zmx, int32 z, NRenderNode* rn, NRect rect)
{
    int i, j;
    
    if (zmx->use == 0) {
        zmx->use = 1;
        zidxMatrix_cellAppend(zmx, 0, z, rn, rect);
        return;
    }
    
    for (i=0; i < zmx->use; i++) {
        if (zmx->panels[i].z == z) {
            zidxMatrix_cellAppend(zmx, i, z, rn, rect);
            return;
        }
        if (zmx->panels[i].z > z)
            break;
    }
    
    if (zmx->use == zmx->max) {
        j = zmx->max;
        zmx->max += ZIDX_PANEL;
        zmx->panels = (NZidxPanel*)NBK_realloc(zmx->panels, sizeof(NZidxPanel) * zmx->max);
        NBK_memset(&zmx->panels[j], 0, sizeof(NZidxPanel) * ZIDX_PANEL);
    }
    
    if (i < zmx->use) {
        for (j = zmx->use; j > i; j--) {
            NBK_memcpy(&zmx->panels[j], &zmx->panels[j-1], sizeof(NZidxPanel));
        }
        NBK_memset(&zmx->panels[i], 0, sizeof(NZidxPanel));
    }
    
    zidxMatrix_cellAppend(zmx, i, z, rn, rect);
    zmx->use++;
}

static void zidxMatrix_appendFrame(NZidxMatrix* zmx, NRenderNode* parent, NRenderNode* rn, NRect rect)
{
    int i, j;
    int z_index;
    nbool found = N_FALSE;
    
    for (i=0; i < zmx->use; i++) {
        for (j=0; j < zmx->panels[i].use; j++) {
            if (zmx->panels[i].cells[j].rn == parent) {
                z_index = zmx->panels[i].z;
                found = N_TRUE;
                break;
            }
        }
    }
    
    if (found)
        zidxMatrix_append(zmx, z_index, rn, rect);
}

#if DUMP_FUNCTIONS
static void zidxMatrix_dump(NZidxMatrix* zmx, NPage* page)
{
    int i, j;
    char buf[64];
    dump_return(page);
    dump_char(page, "===== z-index martix =====", -1);
    dump_int(page, zmx->use);
    dump_int(page, zmx->max);
    dump_return(page);
    for (i=0; i < zmx->use; i++) {
        sprintf(buf, "%8ld %3ld %3ld |", zmx->panels[i].z, zmx->panels[i].use, zmx->panels[i].max);
        dump_char(page, buf, -1);
        for (j=0; j < zmx->panels[i].use; j++) {
            dump_NRect(page, &zmx->panels[i].cells[j].rect);
        }
        dump_return(page);
    }
    dump_flush(page);
}
#endif

NView* view_create(void* page)
{
    NPage* p = (NPage*)page;
    NView* v = (NView*)NBK_malloc0(sizeof(NView));
    N_ASSERT(v);
    
    v->page = page;
    
    if (p->main) {
        v->imgPlayer = imagePlayer_create(v);
        v->ownPlayer = 1;
    }
    else {
        while (p->parent)
            p = p->parent;
        v->imgPlayer = p->view->imgPlayer;
    }
    
    return v;
}

void view_delete(NView** view)
{
    NView* v = *view;
    NPage* p = (NPage*)v->page;

    if (p->main && v->ownPlayer) {
        imagePlayer_delete(&v->imgPlayer);
    }
    
    view_clear(v);
    
    if (v->zidx)
        zidxMatrix_delete(&v->zidx);
    
    NBK_free(v);
    *view = N_NULL;
}

void view_begin(NView* view)
{
    NPage* p = (NPage*)view->page;
    
    view_clear(view);

    view->stat = layoutStat_create();
    view->lastChild = N_NULL;
    view->optWidth = 0;
    
    view->paintMaxTime = -1;
    view->drawPic = view->drawBgimg = 1;

    if (p->main && view->ownPlayer) {
        imagePlayer_reset(view->imgPlayer);
        view->imgPlayer->cache = p->cache;
    }
    
    if (view->zidx)
        zidxMatrix_delete(&view->zidx);

    view->_r_no = 0;
}

static void rtree_delete_node_update_implayer(NView* view, NRenderNode* rn)
{
    int16 iid = -1;
    
    if (rn->type == RNT_IMAGE) {
        iid = ((NRenderImage*)rn)->iid;
    }
    else if (rn->type == RNT_INPUT) {
        iid = ((NRenderInput*)rn)->iid;
    }
    else if (rn->type == RNT_OBJECT) {
        iid = ((NRenderObject*)rn)->iid;
    }
    
    if (iid != -1) {
        imagePlayer_invalidateId(view->imgPlayer, iid);
    }
}

static void rtree_delete_tree(NView* view, NRenderNode* root, nbool updateMp)
{
    int16 top = -1;
    NRenderNode** sta = view->stat->tre;
    NRenderNode* n = root;
    NRenderNode* t = N_NULL;
    
    while (n) {
        if (n->child) {
            sta[++top] = n;
            n = n->child;
        }
        else {
            if (n == root)
                break;
            t = n;
            if (n->next)
                n = n->next;
            else {
                n = sta[top--];
                n->child = N_NULL;
            }
            
            if (updateMp)
                rtree_delete_node_update_implayer(view, t);
            renderNode_delete(&t);
        }
    }
    if (n) {
        if (updateMp)
            rtree_delete_node_update_implayer(view, n);
        renderNode_delete(&n);
    }
}

//#define VIEW_TEST_RELEASE
void view_clear(NView* view)
{
#ifdef VIEW_TEST_RELEASE
    uint32 t_consume;
#endif
    
    if (view->stat == N_NULL)
        return;

#ifdef VIEW_TEST_RELEASE
    t_consume = NBK_currentMilliSeconds();
#endif
    
    rtree_delete_tree(view, view->root, N_FALSE);
    
    layoutStat_delete(&view->stat);
    
    view->root = N_NULL;
    view->focus = N_NULL;
    
#ifdef VIEW_TEST_RELEASE
    dump_char(view->page, "view clear", -1);
    dump_uint(view->page, NBK_currentMilliSeconds() - t_consume);
    dump_return(view->page);
#endif
}

static void view_insert_as_child(NRenderNode** parent, NRenderNode* lastChild, NRenderNode* rn)
{
    if (*parent == N_NULL) {
        *parent = rn;
    }
    else {
        if (lastChild == N_NULL) {
            (*parent)->child = rn;
            rn->parent = *parent;
        }
        else {
            lastChild->next = rn;
            rn->parent = *parent;
            rn->prev = lastChild;
        }
    }
}

static void view_push_node(NView* view, NRenderNode* node)
{
    N_ASSERT(view->stat->tp < MAX_TREE_DEPTH);
    view->stat->tre[++view->stat->tp] = node;
    view->lastChild = node->child;
}

static void view_pop_node(NView* view)
{
    N_ASSERT(view->stat->tp >= 0);
    view->lastChild = view->stat->tre[view->stat->tp--];
}

// building render tree
void view_processNode(NView* view, NNode* node)
{
    NPage* page = (NPage*)view->page;
    NRenderNode* rn = N_NULL;
    
    if (node->id > TAGID_CLOSETAG) {
        if (renderNode_has(node, page->doc->type) && !node_is_single(node->id)) {
            view_pop_node(view);
        }
        return;
    }
    
    if (node->id == TAGID_TEXT && view->root == N_NULL) {
        // ignore text not belong to any block.
        return;
    }
    
    rn = renderNode_create(view, node);
    if (rn == N_NULL) // no need render, or mem insufficent?
        return;
    node->render = rn;

    rn->_no = ++view->_r_no;
    
    if (node->id == TAGID_TEXT || node_is_single(node->id)) {
        
        if (view->stat->tp < 0) {
            node->render = N_NULL;
            renderNode_delete(&rn);
            return;
        }
        
        // don't push text node, because it has no close status.
        view_insert_as_child(&view->stat->tre[view->stat->tp], view->lastChild, rn);
        view->lastChild = rn;
        return;
    }
    
    if (view->root == N_NULL) {
        view_insert_as_child(&view->root, N_NULL, rn);
        
        // initialize width
        rect_set(&view->root->r, 0, 0, ((view->optWidth > 0) ? view->optWidth : view->scrWidth), 0);
        view->root->needLayout = 1;
    }
    else {
        view_insert_as_child(&view->stat->tre[view->stat->tp], view->lastChild, rn);
    }
    view_push_node(view, rn);
}

#if DUMP_FUNCTIONS
static void view_dump_render_node_header(void)
{
    dump_char(g_dp, "level", -1);
    dump_char(g_dp, "type", -1);
    dump_char(g_dp, "tag", -1);
    dump_char(g_dp, "rect", -1);
    dump_char(g_dp, "width", -1);
    dump_char(g_dp, "height", -1);
    dump_char(g_dp, "text", -1);
    dump_return(g_dp);
}

static void view_dump_render_node(const NView* view, const NRenderNode* r, nbool close)
{
    NNode* n;
    char buf[128];
    int i, j;

    for (i=-1, j=0; i < view->stat->lp; i++, j++)
        buf[j] = '\t';

    i = sprintf(&buf[j], "%d|%d %s [%d]", view->stat->lp, view->stat->asp, (close) ? "<--" : "-->", r->_no);
    j += i;
    
    switch (r->type) {
    case RNT_BLOCK:
        nbk_strcpy(&buf[j], "BLOCK/");
        break;
    case RNT_INLINE:
        nbk_strcpy(&buf[j], "INLINE/");
        break;
    case RNT_INLINE_BLOCK:
        nbk_strcpy(&buf[j], "INL-BLK/");
        break;
    case RNT_A:
        nbk_strcpy(&buf[j], "A/");
        break;
    case RNT_TEXT:
        nbk_strcpy(&buf[j], "TEXT/");
        break;
    case RNT_TEXTPIECE:
        nbk_strcpy(&buf[j], "T-PIECE/");
        break;
    case RNT_IMAGE:
        nbk_strcpy(&buf[j], "IMAGE/");
        break;
    case RNT_INPUT:
        nbk_strcpy(&buf[j], "INPUT/");
        break;
    case RNT_TEXTAREA:
        nbk_strcpy(&buf[j], "TEXTAREA/");
        break;
    case RNT_SELECT:
        nbk_strcpy(&buf[j], "SELECT/");
        break;
    case RNT_BR:
        nbk_strcpy(&buf[j], "BR/");
        break;
    case RNT_HR:
        nbk_strcpy(&buf[j], "HR/");
        break;
    case RNT_TABLE:
        nbk_strcpy(&buf[j], "TABLE/");
        break;
    case RNT_TR:
        nbk_strcpy(&buf[j], "TR/");
        break;
    case RNT_TD:
        nbk_strcpy(&buf[j], "TD/");
        break;
    case RNT_BLANK:
        nbk_strcpy(&buf[j], "BLANK/");
        break;
    default:
        nbk_strcpy(&buf[j], "---/");
        break;
    }
    j = nbk_strlen(buf);
    
    n = (NNode*)r->node;
    if (n)
        nbk_strcpy(&buf[j], (char*)xml_getTagNames()[n->id]);

    j = nbk_strlen(buf);
    if (r->display == CSS_DISPLAY_NONE)
        buf[j++] = '*';

    dump_char(g_dp, buf, j);
    
    dump_NRect(g_dp, &r->og_r);
    dump_NRectSides(g_dp, &r->r);
    dump_NRectSides(g_dp, &r->aa_r);
    
    if (r->type == RNT_TEXTPIECE) {
        NRenderTextPiece* rtp = (NRenderTextPiece*)r;
        if (rtp->len)
            dump_wchr(g_dp, rtp->text, rtp->len);
    }
    
    dump_return(g_dp);
    dump_flush(g_dp);
}

void view_dump_render_tree_2(NView* view, NRenderNode* root)
{
    NPage* page = (NPage*)view->page;
    NRenderNode* sta[MAX_TREE_DEPTH];
    char* level = (char*)NBK_malloc(512);
    int lv = -1;
    NRenderNode* r = root;
    NNode* n;
    nbool ignore = N_FALSE;
    int i, j;
    char* val;
    
    dump_return(g_dp);
    dump_char(g_dp, "===== render tree =====", -1);
    dump_return(g_dp);
    
    while (r) {
        if (!ignore) {
            sta[lv+1] = r;
            for (i=1, j=0; i <= lv+1; i++) {
                
                if (sta[i]->next)
                    level[j++] = '|';
                else {
                    if (sta[i] == r)
                        level[j++] = '`';
                    else
                        level[j++] = ' ';
                }
                
                if (sta[i] == r) {
                    level[j++] = '-';
                    level[j++] = '-';
                }
                else {
                    level[j++] = ' ';
                    level[j++] = ' ';
                }
                
                level[j++] = ' ';
            }

            i = sprintf(&level[j], "[%d]", r->_no);
            j += i;

            switch (r->type) {
            case RNT_BLOCK:
                nbk_strcpy(&level[j], "block");
                j += 5;
                break;
            case RNT_INLINE:
                nbk_strcpy(&level[j], "inline");
                j += 6;
                break;
            case RNT_INLINE_BLOCK:
                nbk_strcpy(&level[j], "inl-blk");
                j += 7;
                break;
            case RNT_TEXT:
                nbk_strcpy(&level[j], "text");
                j += 4;
                break;
            case RNT_TEXTPIECE:
                nbk_strcpy(&level[j], "t-piece");
                j += 7;
                break;
            case RNT_IMAGE:
                nbk_strcpy(&level[j], "image");
                j += 5;
                break;
            case RNT_BR:
                nbk_strcpy(&level[j], "br");
                j += 2;
                break;
            case RNT_INPUT:
                nbk_strcpy(&level[j], "input");
                j += 5;
                break;
            case RNT_HR:
                nbk_strcpy(&level[j], "hr");
                j += 2;
                break;
            case RNT_SELECT:
                nbk_strcpy(&level[j], "select");
                j += 6;
                break;
            case RNT_BLANK:
                nbk_strcpy(&level[j], "blank");
                j += 5;
                break;
            case RNT_A:
                nbk_strcpy(&level[j], "a");
                j += 1;
                break;
            case RNT_TABLE:
                nbk_strcpy(&level[j], "table");
                j += 5;
                break;
            case RNT_TR:
                nbk_strcpy(&level[j], "tr");
                j += 2;
                break;
            case RNT_TD:
                nbk_strcpy(&level[j], "td");
                j += 2;
                break;
            }
            
            n = (NNode*)r->node;
            if (n) {
                level[j++] = '/';
                nbk_strcpy(&level[j], xml_getTagNames()[n->id]);
                j += nbk_strlen(xml_getTagNames()[n->id]);

                val = attr_getValueStr(n->atts, ATTID_CLASS);
                if (val) {
                    level[j++] = ' ';
                    nbk_strcpy(&level[j], val);
                    j += nbk_strlen(val);
                }
            }
            
            if (r->type == RNT_BLOCK && ((NRenderBlock*)r)->overflow == CSS_OVERFLOW_HIDDEN) {
                nbk_strcpy(&level[j], " hidden");
                j += 7;
            }
            
            level[j++] = '\t';
            nbk_strcpy(&level[j], (r->needLayout) ? " L" : " n");
            j += 2;
            nbk_strcpy(&level[j], (r->display) ? " D" : " h");
            j += 2;
            
            dump_char(g_dp, level, j);
            dump_NRect(g_dp, &r->og_r);
            dump_NRectSides(g_dp, &r->r);
            dump_NRectSides(g_dp, &r->aa_r);
			//if (r->type == RNT_TEXT)
			//	dump_NRectSides(g_dp, &((NRenderText*)r)->clip);
			//if (r->type == RNT_IMAGE)
			//	dump_NRectSides(g_dp, &((NRenderImage*)r)->clip);
            
            if (r->type == RNT_TEXTPIECE) {
                NRenderTextPiece* rtp = (NRenderTextPiece*)r;
                dump_wchr(g_dp, rtp->text, rtp->len);
            }
            
            dump_return(g_dp);
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == root)
                break;
            if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    NBK_free(level);
    
    dump_flush(g_dp);
}

static void view_dump_dom_node(const NView* view, const NNode* node)
{
    char level[64];
    char ch;
    int i, lv;
    lv = view->stat->lp;
    if (node->id == TAGID_TEXT)
        lv++;
    for (i=0; i <= lv; i++) {
        ch = (i == lv) ? '+' : ' ';
        level[i] = ch;
    }
    level[i++] = '-';
    level[i++] = ' ';
    nbk_strcpy(&level[i], xml_getTagNames()[node->id]);
    dump_char(g_dp, level, -1);
    if (node->id == TAGID_TEXT)
        dump_wchr(g_dp, node->d.text, node->len);
    dump_return(g_dp);
}
#endif

void view_layout(NView* view, NRenderNode* root, nbool force)
{
    NRenderNode* r = root;
    nbool ignore = N_FALSE;
    NStyle style;
    NPage* page = (NPage*)view->page;
    NRect clips[MAX_TREE_DEPTH];
    nbool care_ovfl = N_TRUE; // 是否计算overflow区域
    uint32 __start, __count = 0;
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    if (root == N_NULL)
        return;

    __start = NBK_currentMilliSeconds();
    //dump_char(page, "layout --->", -1);
    
    page->workState = NEWORK_LAYOUT;

    rect_set(&clips[0], 0, 0, N_MAX_COORD, N_MAX_COORD);

    rect_set(&view->overflow, 0, 0, 0, 0);
    if (force || view->root->needLayout) {
        rect_set(&view->root->r, 0, 0, ((view->optWidth > 0) ? view->optWidth : view->scrWidth), 0);
        if (page->settings && page->settings->selfAdaptionLayout)
            view->root->r.r = view->scrWidth;
    }
    
    view->stat->lp = -1;
    view->stat->asp = -1;
    view->stat->sheet = page->sheet;
    layoutStat_push(view->stat, N_NULL);
    layoutStat_init(view->stat, rect_getWidth(&view->root->r));
    while (r) {
        
        if (!ignore) {
            NNode* node = (NNode*)r->node;
            
            style_init(&style);
            if (node) {
                node_calcStyle(view, node, &style);
                if (r->parent == N_NULL) {
                    style.margin.l = 2;
                    style.margin.t = 2;
                    style.margin.r = 2;
                    style.margin.b = 2;
                }
            }
            
            // request background image
            if (r->bgiid == -1 && style.bgImage) {
                r->bgiid = imagePlayer_getId(view->imgPlayer, style.bgImage, NEIT_CSS_IMAGE);
            }
            
            if (style.z_index != 0 && r->type == RNT_BLOCK)
                r->zindex = 1;
        }
        
        style.view = view;
        style.clip = &clips[view->stat->lp + 1];
        if (!ignore)
            renderNode_transform(r, &style);
        r->Layout(view->stat, r, &style, force);
        if (!r->display) {
            care_ovfl = !care_ovfl; // fixme: 仅仅只能用于一级
        }
        __count++;

        //view_dump_render_node(view, r, ignore);
        //dump_NRect(page, style.clip);
        //dump_return(page);

        if (!ignore && r->child) {
            view->stat->lay[++view->stat->lp] = r;
            r = r->child;
            clips[view->stat->lp + 1] = clips[view->stat->lp];
        }
        else {
            if (r->child && care_ovfl /*&& !ignore*/) {
                if (rect_getWidth(&r->r) && rect_getHeight(&r->r))
                    rect_unite(&view->overflow, &r->r);
            }

            ignore = N_FALSE;
            if (r == root)
                break;
            else if (r->next) {
                r = r->next;
                clips[view->stat->lp + 1] = clips[view->stat->lp];
            }
            else {
                // return to parent
                r = view->stat->lay[view->stat->lp--];
                ignore = N_TRUE;
            }
        }
    }
    layoutStat_pop(view->stat);
    
    // normalized
    if (view->overflow.l < 0) view->overflow.l = 0;
    if (view->overflow.t < 0) view->overflow.t = 0;
    
    if (page->main) {
        //dump_char(page, "element overflow", -1);
        //dump_NRect(page, &view->overflow);
        //dump_return(page);
    }
    else {
        NPage* mp = page_getMainPage(page);
        rect_unite(&mp->view->overflow, &view->overflow);
    }

    //dump_uint(page, NBK_currentMilliSeconds() - __start);
    //dump_uint(page, __count);
    //dump_return(page);
    
    page->workState = NEWORK_IDLE;
    
#if DUMP_RENDER_TREE
    view_dump_render_tree_2(view, view->root);
#endif
}

NRect view_getRectWithOverflow(NView* view)
{
    NRect r = {0, 0, 0, 0};
    if (view->root) {
        r = view->root->r;
        rect_unite(&r, &view->overflow);
    }
    return r;
}

coord view_width(NView* view)
{
    if (view->root == N_NULL) {
        return view->scrWidth;
    }
    else {
        NRect r = view_getRectWithOverflow(view);
        return rect_getWidth(&r);
    }
}

coord view_height(NView* view)
{
    coord h = 0;

    if (view->root) {
        NRect r = view_getRectWithOverflow(view);
        h = rect_getHeight(&r);
    }

    return h;
}

void view_paint(NView* view, NRenderRect* rect)
{
    NRenderNode* r;
    NPage* page = (NPage*)view->page;
    nbool ignore = N_FALSE;
    NStyle style;
    float zoom;
    uint32 paintStart;
    int sampling;
    NRenderNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    
#if TEST_PAINT_TIME
    uint32 t_consume;
    int t_times = 0, t_total = 0;
#endif
        
    if (view->root == N_NULL)
        return;
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    page->workState = NEWORK_PAINT;
    
    paintStart = NBK_currentMilliSeconds();
    sampling = 0;
    
#if TEST_PAINT_TIME
    t_consume = NBK_currentMilliSeconds();
#endif
    
    zoom = history_getZoom((NHistory*)page->history);
    r = view->root;
    while (r && !r->needLayout) {
        
        if (!ignore) {
            style_init(&style);
            style.view = view;
            style.dv = view_getRectWithOverflow(view);
            style.zoom = zoom;
            style.drawPic = view->drawPic;
            style.drawBgimg = view->drawBgimg;
            if (r->display) {
                sampling++;
                if (sampling == PAINT_TIME_SAMPLING) {
                    sampling = 0;
                    if (view->paintMaxTime > 0) {
                        uint32 now = NBK_currentMilliSeconds();
                        if (now - paintStart > view->paintMaxTime - view->paintTime) {
                            view->paintTime += now - paintStart;
                            break;
                        }
                    }
                }
                r->Paint(r, &style, rect);
            }
            else if (r->type == RNT_BLOCK)
                ignore = N_TRUE;
            
#if TEST_PAINT_TIME
            t_total++;
            if (style.test)
                t_times++;
#endif
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == view->root)
                break;
            else if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
#if TEST_PAINT_TIME
    if (page->doc->finish) {
        dump_char(page, "view paint", -1);
        dump_uint(page, NBK_currentMilliSeconds() - t_consume);
        dump_uint(page, t_times);
        dump_uint(page, t_total);
        dump_return(page);
    }
#endif
    
    page->workState = NEWORK_IDLE;
}

#define MAX_FOCUS_AREA  16

typedef struct _NTrPaintFocus {
    
    NView*  view;
    NRect*  rect;
    NStyle  style;
    NRect   area[MAX_FOCUS_AREA];
    int     areaNum;
    coord   top;
    coord   bottom;
    coord   center;
    nbool    absLayout;
    int     test;
    
} NTrPaintFocus;

static int vw_paint_focus(NNode* node, void* user, nbool* ignore)
{
    NRenderNode* r = (NRenderNode*)node->render;
    
    if (r) {
        NTrPaintFocus* tpf = (NTrPaintFocus*)user;
        if (tpf->style.belongA && (r->type == RNT_BLOCK || r->type == RNT_BLANK))
            return 0;
        tpf->style.highlight = (r->type == RNT_BLOCK || tpf->style.ne_fold) ? 0 : 1;
        r->Paint(r, &tpf->style, tpf->rect);
    }
    
    return 0;
}

static int calc_focus_area(NTrPaintFocus* task, NRect* rect)
{
    if (task->areaNum == -1) {
        task->areaNum = 0;
        task->area[0] = *rect;
        task->top = rect->t;
        task->bottom = rect->b;
        task->center = rect->t + ((rect->b - rect->t) >> 1);
    }
    else {
        if (   rect->t == task->top
            || rect->b == task->bottom
            || (rect->t <= task->center && rect->b >= task->center)) {
            rect_unite(&task->area[task->areaNum], rect);
        }
        else {
            if (task->areaNum > 0) {
                nbool l, r;
                coord dx, dy;
                l = r = N_FALSE;
                dx = task->area[task->areaNum].l - task->area[task->areaNum-1].l;
                dy = task->area[task->areaNum].r - task->area[task->areaNum-1].r;
                if (N_ABS(dx) < 10)
                    l = N_TRUE;
                if (N_ABS(dy) < 10)
                    r = N_TRUE;
                if (l && r) {
                    rect_unite(&task->area[task->areaNum-1], &task->area[task->areaNum]);
                    task->areaNum--;
                }
            }
            if (task->areaNum == MAX_FOCUS_AREA - 1)
                return 1;
            task->areaNum++;
            task->area[task->areaNum] = *rect;
            task->top = rect->t;
            task->bottom = rect->b;
            task->center = rect->t + ((rect->b - rect->t) >> 1);
        }
    }
    return 0;
}

static int vw_paint_focus_get_area(NNode* node, void* user, nbool* ignore)
{
    int ret = 0;
    NRenderNode* r = (NRenderNode*)node->render;
    NTrPaintFocus* task = (NTrPaintFocus*)user;
    
    if (r) {
        coord x, y;
        NRect pr;
        
        if (task->absLayout)
            x = y = 0;
        else
            renderNode_getAbsPos(r->parent, &x, &y);
        
        if (r->type == RNT_TEXT) {
            NRect cl;
            NRenderText* rt = (NRenderText*)r;
            cl = rt->clip;
            rect_move(&cl, cl.l + x, cl.t + y);
            if (task->absLayout) {
                pr = r->r;
                rect_move(&pr, pr.l + x, pr.t + y);
                rect_intersect(&pr, &cl);
                if (rect_getWidth(&pr) > 0 && rect_getHeight(&pr) > 0)
                    ret = calc_focus_area(task, &pr);
            }
            else {
                NRenderNode* rtp = r->child;
                while (rtp) {
                    pr = rtp->r;
                    rect_move(&pr, pr.l + x, pr.t + y);
                    rect_intersect(&pr, &cl);
                    if (rect_getWidth(&pr) > 0 && rect_getHeight(&pr) > 0)
                        ret = calc_focus_area(task, &pr);
                    rtp = rtp->next;
                }
            }
            *ignore = N_TRUE;
        }
        else if (   r->type != RNT_A
                 && r->type != RNT_BLANK
                 && r->type != RNT_BLOCK
                 && r->type != RNT_INLINE
                 && r->type != RNT_BR )
        {
            pr = r->r;
            if (task->absLayout) {
                if (r->type == RNT_IMAGE) {
                    NRenderImage* ri = (NRenderImage*)r;
                    rect_intersect(&pr, &ri->clip);
                }
            }
            rect_move(&pr, pr.l + x, pr.t + y);
            ret = calc_focus_area(task, &pr);
        }
    }
    
    return ret;
}

void view_paintFocus(NView* view, NRect* rect)
{
    NColor focusColor = {0x03, 0x5c, 0xfe, 0xff};
    
    NPage* page = (NPage*)view->page;
    NTrPaintFocus task;
    nbool isAnchor = N_FALSE;
    
    if (view->focus == N_NULL)
        return;
    
    switch (view->focus->id) {
    case TAGID_A:
    case TAGID_ANCHOR:
    case TAGID_SPAN:
    case TAGID_BUTTON:
    case TAGID_INPUT:
    case TAGID_TEXTAREA:
    case TAGID_SELECT:
    case TAGID_IMG:
    case TAGID_DIV:
    case TAGID_TC_ATTACHMENT:
        break;
    default:
        return;
    }
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    page->workState = NEWORK_PAINT;

    NBK_memset(&task, 0, sizeof(NTrPaintFocus));
    task.view = view;
    task.rect = rect;
    style_init(&task.style);
    task.style.view = view;
    task.style.dv = view_getRectWithOverflow(view);
    task.style.highlight = 1;
    task.style.color = colorWhite;
    task.style.background_color = focusColor;
    task.style.zoom = history_getZoom((NHistory*)page->history);
    task.areaNum = -1;
    task.absLayout = N_FALSE;
    
    task.test = 0;

    if (   view->focus->id == TAGID_A
        || view->focus->id == TAGID_ANCHOR
        || view->focus->id == TAGID_TC_ATTACHMENT )
        isAnchor = N_TRUE;
    
    if (view->focus->id == TAGID_DIV) {
        NRenderNode* r = (NRenderNode*)view->focus->render;
        NNode* n = (NNode*)r->node;
        if (attr_getValueStr(n->atts, ATTID_NE_DISPLAY)) {
            NColor bg = {0x62, 0x92, 0xD2, 0xff};
            task.style.ne_fold = 1;
            task.style.highlight = 0;
            task.style.background_color = bg;
            node_traverse_depth(view->focus, vw_paint_focus, N_NULL, &task);
        }
        else
            r->Paint(r, &task.style, rect);
    }
    else if (isAnchor && view->focus->child == N_NULL) {
        NRenderNode* r = (NRenderNode*)view->focus->render;
        if (r)
            r->Paint(r, &task.style, rect);
    }
    else if (isAnchor) {
        int i;
        NRect pr, cl;
        NColor fill = {0x60, 0xbe, 0xfd, 255};
        coord bold = FOCUS_RING_BOLD;
        coord da = bold >> 1;
        
        // calc focus area
        node_traverse_depth(view->focus, vw_paint_focus_get_area, N_NULL, &task);
        
        if (task.areaNum == -1 && task.absLayout) { // when A hasn't any valid child area, draw itself.
            NRenderNode* r = (NRenderNode*)view->focus->render;
            pr = r->r;
            rect_toView(&pr, task.style.zoom);
            rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
            renderNode_drawFocusRing(pr, task.style.ctlFocusColor, page->platform);
            page->workState = NEWORK_IDLE;
            return;
        }
        
        // adjust focus area
        for (i=0; i < task.areaNum; i++) {
            if (i + 1 == task.areaNum) {
                if (task.areaNum == 1 && task.area[i+1].r <= task.area[i].l)
                    break;
                task.area[i+1].t = task.area[i].b;
            }
            else
                task.area[i].b = task.area[i+1].t;
        }

        cl.l = cl.t = 0 /*-2*/;
        cl.r = rect_getWidth(rect) /*+ 2*/;
        cl.b = rect_getHeight(rect) /*+ 2*/;
        NBK_gdi_setClippingRect(page->platform, &cl);
        
        // fill background
        NBK_gdi_setBrushColor(page->platform, &fill);
        for (i=0; i <= task.areaNum; i++) {
            rect_toView(&task.area[i], task.style.zoom);
            rect_move(&task.area[i], task.area[i].l - rect->l, task.area[i].t - rect->t);
            NBK_gdi_fillRect(page->platform, &task.area[i]);
        }

        // draw borders
        NBK_gdi_setBrushColor(page->platform, &task.style.background_color);
        for (i=0; i <= task.areaNum; i++) {
            
            // draw left side
            pr = task.area[i];
            pr.l -= bold;
            pr.r = pr.l + bold;
            if (i == 0 || task.area[i].l < task.area[i-1].l)
                pr.t -= da;
            if (i == task.areaNum || task.area[i].l < task.area[i+1].l)
                pr.b += da;
            NBK_gdi_fillRect(page->platform, &pr);
            
            // draw right side
            pr = task.area[i];
            pr.l = pr.r;
            pr.r = pr.l + bold;
            if (i == 0 || task.area[i].r > task.area[i-1].r)
                pr.t -= da;
            if (i == task.areaNum || task.area[i].r > task.area[i+1].r)
                pr.b += da;
            NBK_gdi_fillRect(page->platform, &pr);
            
            // draw up side
            pr = task.area[i];
            pr.t -= bold;
            pr.b = pr.t + bold;
            if (i == 0) {
                pr.l -= da;
                pr.r += da;
                NBK_gdi_fillRect(page->platform, &pr);
            }
            else {
                if (task.area[i-1].l > task.area[i].r) {
                    pr.l -= da;
                    pr.r += da;
                    NBK_gdi_fillRect(page->platform, &pr);
                }
                else {
                    if (task.area[i-1].l > task.area[i].l) {
                        pr.r = task.area[i-1].l;
                        pr.l -= da;
                        NBK_gdi_fillRect(page->platform, &pr);
                    }
                    if (task.area[i-1].r < task.area[i].r) {
                        pr.l = task.area[i-1].r;
                        pr.r = task.area[i].r;
                        pr.r += da;
                        NBK_gdi_fillRect(page->platform, &pr);
                    }
                }
            }
            
            // draw down side
            pr = task.area[i];
            pr.t = pr.b;
            pr.b = pr.t + bold;
            if (i == task.areaNum) {
                pr.l -= da;
                pr.r += da;
                NBK_gdi_fillRect(page->platform, &pr);
            }
            else {
                if (task.area[i+1].r < task.area[i].l) {
                    pr.l -= da;
                    pr.r += da;
                    NBK_gdi_fillRect(page->platform, &pr);
                }
                else {
                    if (task.area[i].r > task.area[i+1].r) {
                        pr.l = task.area[i+1].r;
                        pr.r += da;
                        NBK_gdi_fillRect(page->platform, &pr);
                    }
                    if (task.area[i].l < task.area[i+1].l) {
                        pr.l = task.area[i].l;
                        pr.r = task.area[i+1].l;
                        pr.l -= da;
                        NBK_gdi_fillRect(page->platform, &pr);
                    }
                }
            }
        }

        NBK_gdi_cancelClippingRect(page->platform);
        
        task.style.belongA = 1;
        node_traverse_depth(view->focus, vw_paint_focus, N_NULL, &task);
    }
    else {
        node_traverse_depth(view->focus, vw_paint_focus, N_NULL, &task);
    }
    
    page->workState = NEWORK_IDLE;
    
//    dump_char(page, "paintFocus", -1);
//    dump_int(page, tpf.test);
//    dump_return(page);
}

void view_paintControl(NView* view, NRect* rect)
{
    NRenderNode* r;
    NPage* page = (NPage*)view->page;
    NStyle style;
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    if (view->focus == N_NULL)
        return;
        
    page->workState = NEWORK_PAINT;

    style_init(&style);
    style.view = view;
    style.dv = view_getRectWithOverflow(view);
    style.zoom = history_getZoom((NHistory*) page->history);

    if (view->focus) {
        r = (NRenderNode*)view->focus->render;
        style.highlight = 1;
        if (   view->focus->id == TAGID_INPUT
            || view->focus->id == TAGID_TEXTAREA ) {
            r->Paint(r, &style, rect);
        }
        else if (view->focus->id == TAGID_SELECT) {
            NRenderSelect* rs = (NRenderSelect*)r;
            rs->modal = 1;
            r->Paint(r, &style, rect);
            rs->modal = 0;
        }
    }

    page->workState = NEWORK_IDLE;
}

void view_paintMainBodyBorder(NView* view, NNode* node, NRect* rect, nbool focused)
{
    NPage* page = (NPage*)view->page;
    NRenderNode* rn = (NRenderNode*)node->render;
    NRect pr, cl;
    
    if (rn == N_NULL)
        return;
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    page->workState = NEWORK_PAINT;
    
    cl.l = cl.t = 0;
    cl.r = rect_getWidth(rect);
    cl.b = rect_getHeight(rect);
    NBK_gdi_setClippingRect(page->platform, &cl);

    pr = rn->r;
    rect_toView(&pr, history_getZoom((NHistory*)page->history));
        
    if (rect_isIntersect(rect, &pr)) {
        NColor clr = { 0xff, 0x8c, 0x0, 0xff };
        int i;
        
        rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

        if (focused) {
            NColor color = {0xff, 0x8c, 0x0, 0x7d};
            NBK_gdi_setBrushColor(page->platform, &color);
            NBK_gdi_fillRect(page->platform, &pr);
        }
        
        NBK_gdi_setPenColor(page->platform, &clr);
        for (i=0; i < 3; i++) {
            NBK_gdi_drawRect(page->platform, &pr);
            rect_grow(&pr, 1, 1);
        }
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
    
    page->workState = NEWORK_IDLE;
}

void view_setFocusNode(NView* view, NNode* node)
{
//    view->focus = node;
    page_setFocusedNode((NPage*)view->page, node);
}

void* view_getFocusNode(NView* view)
{
    return view->focus;
}

void view_picChanged(NView* view, int16 imgId)
{
    NPage* page = (NPage*)view->page;
    NRenderNode* r = view->root;
    nbool ignore = N_FALSE, layout;
    int lv = 0;
    nbool lay[MAX_TREE_DEPTH];
    NRenderNode** sta = view->stat->lay;

    lay[0] = N_FALSE;
    while (r) { // fixme: make change minimized
        
        if (ignore) {
            if (lay[lv]) {
                r->init = 0;
                r->needLayout = 1;
            }
        }
        else {
            layout = N_FALSE;
            if (r->bgiid == imgId) {
                layout = N_TRUE;
            }
            else if (   (r->type == RNT_IMAGE && ((NRenderImage*)r)->iid == imgId)
                     || (r->type == RNT_INPUT && ((NRenderInput*)r)->iid == imgId) )
            {
                layout = N_TRUE;
                lay[lv] = N_TRUE;
            }

            if (layout || lay[lv]) {
                r->init = 0;
                r->needLayout = 1;
            }
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            lay[lv] = lay[lv-1];
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == view->root)
                break;
            if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                lay[lv] = lay[lv+1];
                ignore = N_TRUE;
            }
        }
    }
    
//    dump_return(page);
//    dump_char(page, "pictures changed ---", -1);
//    view_dump_render_tree_2(view, view->root);
    
    doc_viewChange(page->doc);
}

void view_picsChanged(NView* view, NSList* lst)
{
    NPage* page = (NPage*)view->page;
    NRenderNode* r = view->root;
    nbool ignore = N_FALSE, layout;
    int lv = 0;
    nbool lay[MAX_TREE_DEPTH];
    NRenderNode* sta[MAX_TREE_DEPTH];

    lay[0] = N_FALSE;
    while (r) { // fixme: make change minimized
        
        if (ignore) {
            if (lay[lv]) {
                r->init = 0;
                r->needLayout = 1;
            }
        }
        else {
            layout = N_FALSE;
            if (r->type == RNT_IMAGE || r->type == RNT_INPUT) {
                int16 iid = -1, imgid;

                if (r->type == RNT_IMAGE)
                    iid = ((NRenderImage*)r)->iid;
                else if (r->type == RNT_INPUT)
                    iid = ((NRenderInput*)r)->iid;

                imgid = (int16)(int)sll_first(lst);
                while (imgid > 0 && iid != -1) {
                    if (imgid - 1 == iid) {
                        layout = N_TRUE;
                        lay[lv] = N_TRUE;
                        break;
                    }
                    imgid = (int16)(int)sll_next(lst);
                }
            }

            if (layout || lay[lv]) {
                r->init = 0;
                r->needLayout = 1;
            }
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            lay[lv] = lay[lv-1];
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == view->root)
                break;
            if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                lay[lv] = lay[lv+1];
                ignore = N_TRUE;
            }
        }
    }
    
    doc_viewChange(page->doc);
}

// update pictures in current visible view
// imgId == -1, check all gifs
void view_picUpdate(NView* view, int16 imgId)
{
    NPage* page = (NPage*)view->page;
    NHistory* history = (NHistory*)page->history;
    NRenderNode* r;
    nbool ignore = N_FALSE, has;
    int lv = 0;
    NRect vr;
    NRect area;
    NRenderNode** sta = view->stat->lay;
    
    NBK_helper_getViewableRect(page->platform, &vr);
    rect_toDoc(&vr, history_getZoom(history));

    if (page->doc->type == NEDOC_FULL) {
        // is in main body mode
        page = history_curr(history);
        if (page->doc->type == NEDOC_MAINBODY && imgId >= 0) {
            view_picChanged(page->view, imgId);
            return;
        }
    }

    r = page->view->root;
    while (r) {
        
        if (!ignore) {
			if (imgId >= 0) {
				if ((r->bgiid == imgId) ||
					(r->type == RNT_IMAGE && ((NRenderImage*)r)->iid == imgId)) {
						has = renderNode_getAbsRect(r, &area);
						if (has && rect_isIntersect(&vr, &area)) {
							nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
							return;
						}
				}
			}
			else {
				// 检测 gif 更新
				if (r->type == RNT_IMAGE && imagePlayer_isGif(view->imgPlayer, ((NRenderImage*)r)->iid)) {
					has = renderNode_getAbsRect(r, &area);
					if (has && rect_isIntersect(&vr, &area)) {
						nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
						return;
					}
				}
			}
        }
        
        if (r->display == CSS_DISPLAY_NONE)
            ignore = N_TRUE;
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == page->view->root)
                break;
            if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
}

void view_enablePic(NView* view, nbool enable)
{
    if (view->imgPlayer)
        imagePlayer_disable(view->imgPlayer, !enable);
}

void view_stop(NView* view)
{
    if (view->imgPlayer)
        imagePlayer_stopAll(view->imgPlayer);
}

void rtree_delete_node(NView* view, NRenderNode* node)
{
    NRenderNode* parent;
    NRenderNode* prev;
    NRenderNode* next;
    
    if (node == N_NULL)
        return;
    if (node->type >= RNT_LAST) // fixme
        return;
    
    parent = node->parent;
    prev = node->prev;
    next = node->next;

    if (parent && parent->child == node)
        parent->child = next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    
    rtree_delete_tree(view, node, N_TRUE);
}

nbool rtree_replace_node(NView* view, NRenderNode* old, NRenderNode* novice)
{
    NRenderNode *parent, *prev, *next;
    
    if (old == N_NULL || novice == N_NULL)
        return N_FALSE;
    
    parent = old->parent;
    prev = old->prev;
    next = old->next;
    
    novice->parent = parent;
    novice->prev = prev;
    novice->next = next;
    
    if (parent->child == old)
        parent->child = novice;
    if (prev)
        prev->next = novice;
    if (next)
        next->prev = novice;
    
    rtree_delete_tree(view, old, N_TRUE);
    
    return N_TRUE;
}

nbool rtree_insert_node(NRenderNode* node, NRenderNode* novice, nbool before)
{
    if (node == N_NULL || novice == N_NULL)
        return N_FALSE;
    
    novice->parent = node->parent;
    
    if (before) {
        novice->prev = node->prev;
        novice->next = node;
        if (node->prev)
            node->prev->next = novice;
        else
            node->parent->child = novice;
        node->prev = novice;
    }
    else {
        novice->prev = node;
        novice->next = node->next;
        if (node->next)
            node->next->prev = novice;
        node->next = novice;
    }
    
    return N_TRUE;
}

nbool rtree_insert_node_as_child(NRenderNode* parent, NRenderNode* novice)
{
    if (parent == N_NULL || novice == N_NULL)
        return N_FALSE;
    
    parent->child = novice;
    novice->parent = parent;
    
    return N_TRUE;
}

void view_pause(NView* view)
{
    NPage* page = (NPage*)view->page;
    
    if (page->main) {
        imagePlayer_aniPause(view->imgPlayer);
    }
}

void view_resume(NView* view)
{
    NPage* page = (NPage*)view->page;
    
    if (page->main) {
        imagePlayer_aniResume(view->imgPlayer);
    }
}

void view_nodeDisplayChanged(NView* view, NRenderNode* rn, nid display)
{
    NRenderNode** sta;
    NRenderNode* r;
    int lv, max;
    coord dy;
    
    if (rn->type != RNT_BLOCK || rn->display == display)
        return;
    
    if (display == CSS_DISPLAY_NONE)
        dy = -rect_getHeight(&rn->r);
    else
        dy = rect_getHeight(&rn->r);
    rn->display = (uint8)display;
    
    sta = view->stat->lay;
    max = 0;
    r = rn;
    sta[max++] = rn;
    while (r->parent) {
        r = r->parent;
        sta[max++] = r;
    }
    
    lv = 0;
    r = rn;
    while (r) {
        if (r != sta[lv]) {
            //if (r->type == RNT_TEXT)
            //    renderText_adjustPos(r, dy);
            //else
                renderNode_adjustPos(r, dy);
        }
        
        if (r->next) {
            r = r->next;
        }
        else {
            lv++;
            if (lv == max)
                break;
            r = sta[lv];
            r->r.b += dy;
        }
    }
    
//    view_dump_render_tree_2(view, view->root);
}

typedef struct _NTaskAdjustPos {
    NSList* subPages;
    coord   dy;
} NTaskAdjustPos;

static int adjust_y_offset_abs_task(NRenderNode* rn, void* user, nbool* ignore)
{
    NTaskAdjustPos* task = (NTaskAdjustPos*)user;
    
    if (!rn->display) {
        *ignore = N_TRUE;
        return 0;
    }
    
    renderNode_adjustPos(rn, task->dy);

    if (rn->type == RNT_BLANK) {
        NNode* n = (NNode*)rn->node;
        if (n->id == TAGID_FRAME && n->child && task->subPages) {
            NPage* p = (NPage*)sll_first(task->subPages);
            while (p) {
                if (p->doc->root == n->child) {
                    rtree_traverse_depth(p->view->root, adjust_y_offset_abs_task, N_NULL, user);
                    p->view->frameRect = p->view->root->r;
                    break;
                }
                p = (NPage*)sll_next(task->subPages);
            }
        }
    }

    return 0;
}

static void view_adjust_y_offset_abs(NView* view, NRenderNode* root, coord dy)
{
    NTaskAdjustPos task;
    task.subPages = page_getMainPage((NPage*)view->page)->subPages;
    task.dy = dy;
    rtree_traverse_depth(root, adjust_y_offset_abs_task, N_NULL, &task);
}

void view_nodeDisplayChangedFF(NView* view, NRenderNode* rn, nid display)
{
    NRenderNode** sta;
    NRenderNode* r;
    coord dy;
    int lv, max;
    
    if (rn->type != RNT_BLOCK || rn->display == display)
        return;
    
//    view_dump_render_tree_2(view, view->root);
    
    if (display == CSS_DISPLAY_NONE) {
        // convert to coordinates start from 0,0
        view_adjust_y_offset_abs(view, rn, -rn->r.t);
        dy = -rect_getHeight(&rn->r);
    }
    else {
        // get y offset from sibling or parent
        r = rn->prev;
        while (r && r->display == CSS_DISPLAY_NONE)
            r = r->prev;
        if (r)
            dy = r->r.b;
        else {
            dy = rn->parent->r.t;
            if (rn->parent->type == RNT_BLOCK)
				dy += rn->parent->border.t;
        }
        rn->display = (uint8)display;
        view_adjust_y_offset_abs(view, rn, dy);
        dy = rect_getHeight(&rn->r);
    }
    rn->display = (uint8)display;
    
    sta = view->stat->lay;
    r = rn;
    max = -1;
    while (r->parent) {
        r = r->parent;
        sta[++max] = r;
    }
    
    lv = -1;
    r = rn;
    while (r) {
        
        if (r->next) {
            r = r->next;
            if (r->display != CSS_DISPLAY_NONE)
                view_adjust_y_offset_abs(view, r, dy);
        }
        else {
            if (lv == max)
                break;
            r = sta[++lv];
            r->r.b += dy;
        }
    }
    
//    view_dump_render_tree_2(view, view->root);
}

void view_checkFocusVisible(NView* view)
{
    NNode* r = view->focus;
    NRenderNode* render;
    
    while (r) {
        render = (NRenderNode*)r->render;
        if (render && render->display == CSS_DISPLAY_NONE) {
            view_setFocusNode(view, N_NULL);
            return;
        }
        r = r->parent;
    }
}

static NRect get_max_render_rect(NRenderNode* root, NRenderNode** sta)
{
    NRect rect = root->r;
    NRenderNode* r = root;
    int lv = 0;
    nbool ignore = N_FALSE;

    while (r) {
        if (!ignore && r->display) {
            if (r->zindex && r != root)
                ignore = N_TRUE;
            else if (!rect_isEmpty(&r->r)) {
                NRect cr = r->r;
                if (RNT_IMAGE == r->type) {
                    NRenderImage* ri = (NRenderImage*) r;
                    rect_intersect(&cr, &ri->clip);
                }
                else if (RNT_TEXT == r->type) {
                    NRenderText* rt = (NRenderText*) r;
                    rect_intersect(&cr, &rt->clip);
                }

                if (rect_getWidth(&cr) && rect_getHeight(&cr))
                    rect_unite(&rect, &cr);
            }
        }
        
        if (!ignore && r->child) {
            sta[lv++] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == root)
                break;
            else if (r->next)
                r = r->next;
            else {
                r = sta[--lv];
                ignore = N_TRUE;
            }
        }
    }
    
    return rect;
}

typedef struct _NTaskBuildZindex {
    NZidxMatrix*    zidx;
    NView*          view;
} NTaskBuildZindex;

static int zindex_building(NNode* node, void* user, nbool* ignore)
{
    if (node->id == TAGID_DIV) {
        NTaskBuildZindex* task = (NTaskBuildZindex*)user;
        NZidxMatrix* zidx = task->zidx;
        NRenderNode* r = (NRenderNode*)node->render; 
        
        if (r->zindex) {
            NRect rect = get_max_render_rect(r, task->view->stat->lay);
            NStyle style;
            int32 z_index;
            
            style_init(&style);
            node_calcStyle(task->view, node, &style);
            z_index = style.z_index;
            
            r = r->parent;
            while (r) {
                if (r->type == RNT_BLOCK && r->zindex) {
                    style_init(&style);
                    node_calcStyle(task->view, (NNode*)r->node, &style);
                    z_index += style.z_index;
                }
                r = r->parent;
            }

            zidxMatrix_append(zidx, z_index, (NRenderNode*)node->render, rect);
        }
    }
    else if (node->id == TAGID_FRAME && node->child) {
        NNode* n = node->parent;
        NRenderNode* parent = N_NULL;
        while (n) {
            if (n->render) {
                parent = (NRenderNode*)n->render;
                if (parent->zindex)
                    break;
                else
                    parent = N_NULL;
            }
            n = n->parent;
        }
        if (parent) { // add frame to z-matrix
            NTaskBuildZindex* task = (NTaskBuildZindex*)user;
            NZidxMatrix* zidx = task->zidx;
            NPage* page = (NPage*)task->view->page;
            NPage* subPage;
            if (page->subPages) {
                subPage = (NPage*)sll_first(page->subPages);
                while (subPage) {
                    if (node->child == subPage->doc->root) {
                        zidxMatrix_appendFrame(zidx, parent, subPage->view->root, subPage->view->frameRect);
                        break;
                    }
                    subPage = (NPage*)sll_next(page->subPages);
                }
            }
        }
    }
    
    return 0;
}

void view_buildZindex(NView* view)
{
    NPage* page = (NPage*)view->page;
    NTaskBuildZindex task;
    
    if (!page->main)
        return;

    if (view->zidx)
        zidxMatrix_delete(&view->zidx);
    view->zidx = zidxMatrix_create();
    
//    if (view->zidx == N_NULL)
//        view->zidx = zidxMatrix_create();
//    else
//        zidxMatrix_reset(view->zidx);
    
    task.zidx = view->zidx;
    task.view = page->view;
    node_traverse_depth(page->doc->root, zindex_building, N_NULL, &task);
    view->zidx->ready = N_TRUE;
    
//    zidxMatrix_dump(view->zidx, page);
}

nbool view_isZindex(NView* view)
{
    return ((view->zidx && view->zidx->ready) ? N_TRUE : N_FALSE);
}

void view_clearZindex(NView* view)
{
    if (view->zidx)
        zidxMatrix_delete(&view->zidx);
//        zidxMatrix_reset(view->zidx);
}

void view_paintPartially(NView* view, NRenderNode* root, NRect* rect)
{
    NRenderNode* r;
    NPage* page = (NPage*)view->page;
    NStyle style;
    float zoom;
    nbool ignore = N_FALSE;
    NRenderNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    uint32 paintStart;
    int sampling;
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    page->workState = NEWORK_PAINT;
    
    paintStart = NBK_currentMilliSeconds();
    sampling = 0;
    
    zoom = history_getZoom((NHistory*)page->history);
    r = root;
    while (r && !r->needLayout) {
        
        if (!ignore) {
            style_init(&style);
            style.view = view;
            style.dv = view_getRectWithOverflow(view);
            style.zoom = zoom;
            style.drawPic = view->drawPic;
            style.drawBgimg = view->drawBgimg;
            if (r->display) {
                if (r->zindex && r != root) // skip sub-trees which own z-index
                    ignore = N_TRUE;
                else {
                    sampling++;
                    if (sampling == PAINT_TIME_SAMPLING) {
                        sampling = 0;
                        if (view->paintMaxTime > 0) {
                            uint32 now = NBK_currentMilliSeconds();
                            if (now - paintStart > view->paintMaxTime - view->paintTime) {
                                view->paintTime += now - paintStart;
                                break;
                            }
                        }
                    }
                    r->Paint(r, &style, rect);
                }
            }
            else if (r->type == RNT_BLOCK)
                ignore = N_TRUE;
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == root)
                break;
            else if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    page->workState = NEWORK_IDLE;
}

#define CLICK_MIN_PIXEL 20
#define CLICK_EXT_PIXEL 10

static NRect extend_width(NRect rect)
{
    NRect r = rect;
    if (rect_getWidth(&r) < CLICK_MIN_PIXEL) {
        r.l -= CLICK_EXT_PIXEL;
        r.r += CLICK_EXT_PIXEL;
    }
    if (rect_getHeight(&r) < CLICK_MIN_PIXEL) {
        r.t -= CLICK_EXT_PIXEL;
        r.b += CLICK_EXT_PIXEL;
    }
    return r;
}

static NNode* view_find_focus_with_render(
    NRenderNode* root, coord x, coord y, NImagePlayer* player)
{
    NRenderNode* r = root;
    nbool ignore = N_FALSE;
    NRenderNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    NNode* n;
    NNode* focus = N_NULL;
    NNode* emA = N_NULL;
    NRect hot;
    
    while (r) {
        if (!ignore) {
            if (r->display) {
                if (   r->type == RNT_INPUT
                    && (   ((NRenderInput*)r)->type == NEINPUT_CHECKBOX
                        || ((NRenderInput*)r)->type == NEINPUT_RADIO ) )
                    hot = extend_width(r->r);
                else
                    hot = r->r;

                if (r->zindex && r != root) {
                    ignore = N_TRUE;
                }
                else if (rect_hasPt(&hot, x, y)) {
                    switch (r->type) {
                    case RNT_INPUT:
                    case RNT_TEXTAREA:
                    case RNT_SELECT:
                        return (NNode*)r->node;
                    case RNT_A:
                        emA = (NNode*)r->node;
                        if (r->child == N_NULL)
                            return emA;
                        else if (r->child->type == RNT_BLANK && r->child->next == N_NULL)
                            return emA;
                        break;
                    case RNT_IMAGE:
                        n = (NNode*)r->node;

                        if (player) {
                            nid state = imagePlayer_getState(player, ((NRenderImage*)r)->iid);
                            if (state == NEIS_PENDING || state == NEIS_HAS_CACHE)
                                return (NNode*)r->node;
                        }
                        else if (attr_getValueInt32(n->atts, ATTID_TC_IA) > 0)
                            return (NNode*)r->node;
                      
                        if (emA)
                            return emA;
                        break;
                    case RNT_TEXT:
                        if (emA) {
                            //hot = extend_width(r->r);
                            if (rect_hasPt(&hot, x, y))
                                return emA;
                        }
                        break;
                    case RNT_BLOCK:
                    {
                        n = (NNode*)r->node;
                        if (   (n->id == TAGID_DIV && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0)
                            || (((NRenderBlock*)r)->ne_display) )
                            focus = n;
                        break;
                    }
                    default:
                    	break;
                    } // switch
                }
            }
            else
                ignore = N_TRUE;
        }
        
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == root)
                break;
            else if (r->next) {
                if (r->type == RNT_A)
                    emA = N_NULL;
                r = r->next;
            }
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    return focus;
}

NNode* view_findFocusWithZindex(NView* view, coord x, coord y)
{
    int i, j;
    NPage* page = (NPage*)view->page;
    NNode* focus = N_NULL;
    NImagePlayer* player = N_NULL;
    
    if (!page->main)
        return N_NULL;
    
    if (   page->doc->type == NEDOC_FULL
        && ((NSettings*)page->settings)->clickToLoadImage )
        player = page->view->imgPlayer;
    
    for (i = view->zidx->use - 1; i >= 0; i--) {
        if (view->zidx->panels[i].use && view->zidx->panels[i].z > 0) {
            for (j = view->zidx->panels[i].use - 1; j >= 0; j--) {
                if (   view->zidx->panels[i].cells[j].normal == N_FALSE
                    || rect_hasPt(&view->zidx->panels[i].cells[j].rect, x, y)) {
                    focus = view_find_focus_with_render(view->zidx->panels[i].cells[j].rn, x, y, player);
                    if (focus || view->zidx->panels[i].cells[j].normal)
                        return focus;
                }
            }
        }
    }
    
    focus = view_find_focus_with_render(view->root, x, y, player);
    
    if (focus == N_NULL && page->subPages) {
        NPage* p = (NPage*)sll_first(page->subPages);
        while (p) {
            if (p->attach && renderNode_isRenderDisplay(p->view->root)) {
                if (rect_hasPt(&p->view->root->r, x, y))
                    return view_find_focus_with_render(p->view->root, x, y, player);
            }
            p = (NPage*)sll_next(page->subPages);
        }
    }
    
    return focus;
}

NRect view_getNodeRect(NView* view, NNode* node)
{
    NPage* page = (NPage*)view->page;
    NRenderNode* r;
    nbool ignore = N_FALSE;
    NRenderNode* sta[MAX_TREE_DEPTH];
    int lv = -1;
    NRect rect = {N_MAX_COORD, N_MAX_COORD, 0, 0};
    NRect a;
    coord x, y;
    
    if (node == N_NULL)
        return rect;

    r = (NRenderNode*)node->render;
    while (r) {
            
        if (!ignore && r->type != RNT_INLINE && r->type != RNT_TEXT) {
            renderNode_getAbsPos((r->type == RNT_TEXTPIECE) ? r->parent->parent : r->parent, &x, &y);
            a = r->r;
            rect_move(&a, a.l + x, a.t + y);
            rect_unite(&rect, &a);
            ignore = N_TRUE;
        }
            
        if (!ignore && r->child) {
            sta[++lv] = r;
            r = r->child;
        }
        else {
            ignore = N_FALSE;
            if (r == (NRenderNode*)node->render)
                break;
            if (r->next)
                r = r->next;
            else {
                r = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }

    rect_toView(&rect, history_getZoom((NHistory*)page->history));
    return rect;
}

// 按 z 序关系绘制渲染树
void view_paintWithZindex(NView* view, NRect* rect)
{
    int i, j;
    NPage* page = (NPage*)view->page;
    NZidxMatrix* zidx = view->zidx;
    float zoom = history_getZoom((NHistory*)page->history);
    
    if (!page->main)
        return;
    
    view->paintTime = 0;

    // 绘制低于 0 的层
    for (i=0; i < zidx->use && zidx->panels[i].z < 0; i++) {
        for (j=0; j < zidx->panels[i].use; j++) {
            view_paintPartially(view, zidx->panels[i].cells[j].rn, rect);
        }
    }

    // 绘制 0 层
    view_paintPartially(view, view->root, rect);
    if (page->subPages) {
        NPage* p = (NPage*)sll_first(page->subPages);
        while (p) {
            if (p->attach && renderNode_isRenderDisplay(p->view->root)) {
                NRect vr = p->view->frameRect;
                rect_toView(&vr, zoom);
                if (rect_isIntersect(&vr, rect)) {
                    vr.l = rect->l;
                    vr.t = rect->t;
                    vr.r = N_MIN(vr.r, rect->r);
                    vr.b = N_MIN(vr.b, rect->b);
                    if (rect_getWidth(&vr) > 0 && rect_getHeight(&vr) > 0)
                        view_paintPartially(view, p->view->root, &vr);
                }
            }
            p = (NPage*)sll_next(page->subPages);
        }
    }

    // 绘制高于 0 的层
    for (i=0; i < zidx->use; i++) {
        if (zidx->panels[i].z > 0) {
            for (j=0; j < zidx->panels[i].use; j++) {
                view_paintPartially(view, zidx->panels[i].cells[j].rn, rect);
            }
        }
    }
}

void view_paintFocusWithZindex(NView* view, NRect* rect)
{
    NRenderNode* r;
    int32 z_index = 0;
    int i, j;
    
    if (view->focus == N_NULL)
        return;
    
    view_paintFocus(view, rect);
    
    // find panel which focus in.
    r = (NRenderNode*)view->focus->render;
    while (r && r->zindex == 0)
        r = r->parent;
    
    if (r) {
        NNode* n = (NNode*)r->node;
        NStyle style;
        
        style_init(&style);
        node_calcStyle(view, n, &style);
        z_index = style.z_index;
    }
    
    for (i=0; i < view->zidx->use; i++) {
        if (view->zidx->panels[i].z > z_index) {
            for (j=0; j < view->zidx->panels[i].use; j++) {
                view_paintPartially(view, view->zidx->panels[i].cells[j].rn, rect);
            }
        }
    }
}

// Feature: Layout text by specify width.

//typedef struct _TaskBLAdjustRtree {
//    NRect*  box;
//    coord   dy;
//} TaskBLAdjustRtree;
//
//static int bl_adjust_rtree(NRenderNode* render, void* user, nbool* ignore)
//{
//    TaskBLAdjustRtree* task = (TaskBLAdjustRtree*)user;
//    if (render->r.t > task->box->t) {
////        coord l = N_MAX(render->r.l, task->box->l);
////        coord r = N_MIN(render->r.r, task->box->r);
//        if (1/*l < r*/) {
//            if (render->type == RNT_TEXT)
//                renderText_adjustPos(render, task->dy);
//            else
//                renderNode_adjustPos(render, task->dy);
//        }
//    }
//    return 0;
//}
//
//static void bl_adjust_render_y_offset(NView* view, NRect* box, coord dy)
//{
//    TaskBLAdjustRtree task;
//    task.box = box;
//    task.dy = dy;
//    rtree_traverse_depth(view->root, bl_adjust_rtree, N_NULL, &task);
//}
//
//static void bl_extent_parent_height(NRenderNode* render, coord dy)
//{
//    NRenderNode* r = render;
//    while (r) {
//        r->r.b += dy;
//        r = r->parent;
//    }
//}
//
//typedef struct _TaskBLAdjust {
//    NPage*  page;
//    coord   width;
//    int     total;
//} TaskBLAdjust;
//
//static int bl_adjust_text(NNode* node, void* user, nbool* ignore)
//{
//    nbool skip = N_FALSE;
//    
//    if (node->id == TAGID_DIV && attr_getValueInt32(node->atts, ATTID_TC_IA) > 0)
//        skip = N_TRUE;
//    else if (   node->id == TAGID_A
////             || node->id == TAGID_FRAME
//             || node->id == TAGID_FORM
//             || node->id == TAGID_SELECT
//             || node->id == TAGID_TEXTAREA)
//        skip = N_TRUE;
//    
//    if (skip) {
//        *ignore = skip;
//    }
//    else if (node->id == TAGID_TEXT) {
//        TaskBLAdjust* task = (TaskBLAdjust*)user;
//        NRenderNode* r = (NRenderNode*)node->render;
//        NRect old = r->r;
//        NStyle style;
//        coord dy;
//        
//        style.view = task->page->view;
//        renderText_breakLineByWidthFF(r, &style, task->width);
//        
//        dy = r->r.b - old.b;
//        if (dy) {
//            bl_extent_parent_height(r->parent->parent, dy);
//            bl_adjust_render_y_offset(task->page->view, &old, dy);
//            task->total++;
////            dump_wchr(task->page, node->d.text, node->len);
////            dump_return(task->page);
////            return 1;
//        }
//    }
//    
//    return 0;
//}
//
//nbool view_layoutTextFF(NView* view)
//{
//    NPage* page = (NPage*)view->page;
//    NRect vr;
//    TaskBLAdjust task;
//    
//    NBK_helper_getViewableRect(page->platform, &vr);
//    
//    task.page = page;
//    task.width = rect_getWidth(&vr);
//    task.width = task.width - task.width / 10;
//    task.total = 0;
//    node_traverse_depth(page->doc->root, bl_adjust_text, N_NULL, &task);
//    
////    dump_char(page, "adjust text node:", -1);
////    dump_int(page, task.total);
////    dump_return(page);
////    dump_flush(page);
////    
////    view_dump_render_tree_2(view, view->root);
//    
//    return ((task.total > 0) ? N_TRUE : N_FALSE);
//}

float view_getMinZoom(NView* view)
{
    float zoom = 1.0f;

#if !NBK_DISABLE_ZOOM
    if (view->scrWidth > 0) {
        NRect vw;
        coord w = view_width(view);
        
        if (w == 0)
            return zoom;
        
        NBK_helper_getViewableRect(((NPage*)view->page)->platform, &vw);
		zoom = (float)rect_getWidth(&vw) / w;
        
        if (zoom >= 2.0f)
			zoom = 2.0f;
    }
#endif
    
    return zoom;
}

void view_normalizePoint(NView* view, NPoint* pos, NRect va, float zoom)
{
    coord vw = float_imul(view_width(view), zoom);
    coord vh = float_imul(view_height(view), zoom);

    if (pos->x < 0) pos->x = 0;
    if (pos->x + rect_getWidth(&va) >= vw) {
        pos->x = vw - rect_getWidth(&va);
        if (pos->x < 0) pos->x = 0;
    }

    if (pos->y < 0) pos->y = 0;
    if (pos->y + rect_getHeight(&va) >= vh) {
        pos->y = vh - rect_getHeight(&va);
        if (pos->y < 0) pos->y = 0;
    }
}

typedef struct _NTaskFindRender {
    nbool            absLayout;
    coord           x;
    coord           y;
    NRenderNode*    r;
} NTaskFindRender;

static int task_find_render(NRenderNode* r, void* user, nbool* ignore)
{
    NTaskFindRender* task = (NTaskFindRender*)user;
    NRect box;

    if (!r->display || r->zindex) {
        *ignore = N_TRUE;
    }
    else if (r->type == RNT_TEXT && task->absLayout) {
        if (rect_hasPt(&r->r, task->x, task->y)) {
            task->r = r;
            return 1;
        }
    }
    else if (r->type == RNT_TEXTPIECE) {
        renderNode_getAbsRect(r, &box);
        if (rect_hasPt(&box, task->x, task->y)) {
            task->r = r;
            return 1;
        }
    }
    else if (   r->type == RNT_IMAGE
             || r->type == RNT_INPUT
             || r->type == RNT_TEXTAREA
             || r->type == RNT_SELECT ) {
        if (task->absLayout)
            box = r->r;
        else
            renderNode_getAbsRect(r, &box);
        if (rect_hasPt(&box, task->x, task->y)) {
            task->r = r;
            return 1;
        }
    }

    return 0;
}

static NRenderNode* view_find_render_by_pos(NRenderNode* root, coord x, coord y, nbool absLayout)
{
    NTaskFindRender task;
    NBK_memset(&task, 0, sizeof(NTaskFindRender));
    task.absLayout = absLayout;
    task.x = x;
    task.y = y;
    rtree_traverse_depth(root, task_find_render, N_NULL, &task);
    return task.r;
}

NRenderNode* view_findRenderByPos(NView* view, const NPoint* pos)
{
    NPage* page = (NPage*)view->page;
    float zoom = history_getZoom((NHistory*)page->history);
    coord x = float_idiv(pos->x, zoom);
    coord y = float_idiv(pos->y, zoom);
    NRenderNode* find = N_NULL;

    if (view_isZindex(view)) {
        int i, j;
        for (i = view->zidx->use - 1; i >= 0; i--) {
            if (view->zidx->panels[i].use && view->zidx->panels[i].z > 0) {
                for (j = view->zidx->panels[i].use - 1; j >= 0; j--) {
                    if (rect_hasPt(&view->zidx->panels[i].cells[j].rect, x, y)) {
                        find = view_find_render_by_pos(view->zidx->panels[i].cells[j].rn, x, y, N_FALSE);
                        if (find)
                            return find;
                    }
                }
            }
        }
    }

    find = view_find_render_by_pos(view->root, x, y, N_FALSE);

    return find;
}

NERenderKind view_getRenderKind(NRenderNode* rn)
{
    NERenderKind kind = NERK_NONE;
    if (rn) {
        if (   rn->type == RNT_TEXT
            || rn->type == RNT_TEXTPIECE
            || rn->type == RNT_IMAGE )
        {
            NNode* n = (rn->type == RNT_TEXTPIECE) ? (NNode*)rn->parent->node : (NNode*)rn->node;
            while (n) {
                if (n->id == TAGID_A) {
                    kind = (rn->type == RNT_IMAGE) ? NERK_IMAGE_HYPERLINK : NERK_HYPERLINK;
                    break;
                }
                n = n->parent;
            }
            if (kind == NERK_NONE) {
                if (rn->type == RNT_IMAGE)
                    kind = NERK_IMAGE;
                else
                    kind = NERK_TEXT;
            }
        }
        else if (rn->type == RNT_TEXTAREA)
            kind = NERK_EDITOR;
        else if (rn->type == RNT_SELECT)
            kind = NERK_CONTROL;
        else if (rn->type == RNT_INPUT) {
            NRenderInput* ri = (NRenderInput*)rn;
            if (ri->type == NEINPUT_TEXT || ri->type == NEINPUT_PASSWORD)
                kind = NERK_EDITOR;
            else
                kind = NERK_CONTROL;
        }
    }
    return kind;
}

const char* view_getRenderSrc(NRenderNode* rn)
{
    char* src = N_NULL;
    if (rn && rn->type == RNT_IMAGE) {
        NNode* n = (NNode*)rn->node;
        src = attr_getValueStr(n->atts, ATTID_SRC);
    }
    return src;
}
