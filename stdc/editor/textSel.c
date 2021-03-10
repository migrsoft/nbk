#include "textSel.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_ctlPainter.h"
#include "../inc/nbk_gdi.h"
#include "../css/color.h"
#include "../css/css_val.h"
#include "../dom/history.h"
#include "../dom/page.h"
#include "../dom/view.h"
#include "../dom/node.h"
#include "../render/renderText.h"
#include "../render/renderTextPiece.h"
#include "../tools/dump.h"
#include "../tools/str.h"

//#define DUMP_FUNCTIONS

#ifdef DUMP_FUNCTIONS
    #include <stdio.h>
#endif

#define MIN_STEP        16
#define IA_AREA_GROW_X  20
#define IA_AREA_GROW_Y  30
#define SCREEN_EDGE     20
#define COUNT_DA        4

#define RETURN_PTR      (NRenderNode*)0xa

#ifdef DUMP_FUNCTIONS

static void dump_text_list(NTextSel* sel)
{
    NRenderNode* r = (NRenderNode*)dll_first(sel->rlst);
    NRect box;

    while (r) {
        if (r == RETURN_PTR) {
            dump_char(g_dp, "---", 3);
        }
        else {
            dump_int(g_dp, r->_no);

            if (r->type == RNT_TEXT) {
                NNode* n = (NNode*)r->node;
                dump_NRect(g_dp, &r->r);
                dump_wchr(g_dp, n->d.text, n->len);
            }
            if (r->type == RNT_TEXTPIECE) {
                NRenderTextPiece* rtp = (NRenderTextPiece*)r;
                renderNode_getAbsRect(r, &box);
                dump_NRect(g_dp, &box);
                dump_wchr(g_dp, rtp->text, rtp->len);
            }
        }
        dump_return(g_dp);

        r = (NRenderNode*)dll_next(sel->rlst);
    }
}

static void print_sel_area(NTextSel* sel)
{
    char buf[128];
    char* p;
    int i, len;

    p = buf;
    for (i=0; i < 3; i++) {
        len = sprintf(p, "(%d,%d,%d,%d) ", sel->sa[i].l, sel->sa[i].t, sel->sa[i].r, sel->sa[i].b);
        p += len;
    }
    fprintf(stderr, "%s\n", buf);
}

#endif

typedef struct _NTaskFindRenderText {

    nbool    absLayout;
    nbool    addText;
    NDList* rlst;
    int     test_no;

} NTaskFindRenderText;

// 按坐标从上到下、从左到右顺序插入列表
static void append_text_by_pos(NDList* lst, NRenderNode* rn)
{
    NDLNode* n = dll_firstNode(lst);
    NRenderNode* r;
    nbool insert;

    if (n == N_NULL) {
        dll_append(lst, rn);
        return;
    }

    while (n) {
        r = (NRenderNode*)dll_getData(n);

        if (r != RETURN_PTR) {
            insert = N_FALSE;
            if (rn->r.t < r->r.t)
                insert = N_TRUE;
            else if (rn->r.t == r->r.t && rn->r.l < r->r.l)
                insert = N_TRUE;

            if (insert) {
                dll_insertBefore(lst, rn);
                return;
            }
        }

        n = dll_nextNode(lst, n);
    }

    dll_append(lst, rn);
}

// 增加换行标记
static nbool add_return(NRenderNode* r)
{
    switch (r->type) {
    case RNT_BLOCK:
    case RNT_BR:
    case RNT_TR:
        return N_TRUE;
    default:
        return N_FALSE;
    }
}

// 创建可选择文本有序列表
static int find_text(NRenderNode* r, void* user, nbool* ignore)
{
    NTaskFindRenderText* task = (NTaskFindRenderText*)user;

    if (!r->display || r->zindex) {
        *ignore = N_TRUE;
    }
    else if (add_return(r)) {
        if (task->addText) {
            task->addText = N_FALSE;
            if (task->absLayout)
                dll_insertAfter(task->rlst, RETURN_PTR);
            else
                dll_append(task->rlst, RETURN_PTR);
        }
    }
    else if (r->type == RNT_TEXT && task->absLayout) {
        NNode* n = (NNode*)r->node;
        if (n->len > 0) {
            //r->_no = ++task->test_no;
            //dll_append(task->rlst, r);
            append_text_by_pos(task->rlst, r);
            task->addText = N_TRUE;
        }
    }
    else if (r->type == RNT_TEXTPIECE) {
        NRenderTextPiece* rtp = (NRenderTextPiece*)r;
        if (rtp->len > 0) {
            r->_no = ++task->test_no;
            dll_append(task->rlst, r);
            task->addText = N_TRUE;
        }
    }

    return 0;
}

// 确定选择发生的层
static NRenderNode* find_render_root(NView* view, NPoint pos)
{
    NPage* page = (NPage*)view->page;
    float zoom = history_getZoom((NHistory*)page->history);
    coord x = float_idiv(pos.x, zoom);
    coord y = float_idiv(pos.y, zoom);

    if (view_isZindex(view)) {
        int i, j;

        for (i = view->zidx->use - 1; i >= 0; i--) {
            if (view->zidx->panels[i].use && view->zidx->panels[i].z > 0) {
                for (j = view->zidx->panels[i].use - 1; j >= 0; j--) {
                    if (rect_hasPt(&view->zidx->panels[i].cells[j].rect, x, y)) {
                        return view->zidx->panels[i].cells[j].rn;
                    }
                }
            }
        }

        return view->root;
    }
    else
        return view->root;
}

static void build_render_list(NTextSel* sel)
{
    NRenderNode* root = find_render_root(sel->view, sel->lastPos);
    sel->init = N_TRUE;

    if (root) {
        NTaskFindRenderText task;
        NBK_memset(&task, 0, sizeof(NTaskFindRenderText));
        task.absLayout = N_FALSE;
        task.rlst = sel->rlst;
        rtree_traverse_depth(root, find_text, N_NULL, &task);

        if (task.absLayout) { // 重排序号
            NDLNode* n = dll_firstNode(sel->rlst);
            int test_no = 0;
            while (n) {
                NRenderNode* r = (NRenderNode*)dll_getData(n);
                if (r != RETURN_PTR)
                    r->_no = ++test_no;
                n = dll_nextNode(sel->rlst, n);
            }
        }

        //dump_text_list(sel);
    }
}

static wchr* get_text_info(NRenderNode* r, NRect* box, NFontId* fid, int* len)
{
    wchr* txt = N_NULL;

    if (r == RETURN_PTR) {
    }
    else if (r->type == RNT_TEXT) { // 固定布局
        *box = r->r;
        *fid = ((NRenderText*)r)->fontId;
        txt = ((NNode*)r->node)->d.text;
        *len = ((NNode*)r->node)->len;
    }
    else if (r->type == RNT_TEXTPIECE) { // 标准布局
        renderNode_getAbsRect(r, box);
        *fid = ((NRenderText*)r->parent)->fontId;
        txt = ((NRenderTextPiece*)r)->text;
        *len = ((NRenderTextPiece*)r)->len;
    }

    return txt;
}

// 根据水平坐标偏移获取最接近的字符偏移
static int get_offset_in_text(const wchr* txt, int len, NFontId fid, coord x, void* pfd)
{
    int i;
    coord w, ow;
    for (i = ow = 0; i < len; i++) {
        w = NBK_gdi_getCharWidth(pfd, fid, *(txt + i));
        if (x >= ow && x < ow + w)
            break;
        ow += w;
    }
    return ((i < len) ? i : len - 1);
}

static NDLNode* find_selected_node(NTextSel* sel, NPoint pos, int* num, NDLNode** prev, NDLNode** next)
{
    NPage* page = (NPage*)sel->view->page;
    float zoom = history_getZoom((NHistory*)page->history);
    NDLNode* n;
    NRenderNode *r;
    NRect box;
    NFontId fid;
    wchr* txt;
    int len;

    // 转换到文档坐标
    pos.x = float_idiv(pos.x, zoom);
    pos.y = float_idiv(pos.y, zoom);

    *prev = *next = N_NULL;
    n = dll_firstNode(sel->rlst);
    while (n) {

        r = (NRenderNode*)dll_getData(n);
        txt = get_text_info(r, &box, &fid, &len);

        if (txt) {
            if ((box.l > pos.x && box.t > pos.y) || box.t > pos.y) {
                *next = n;
                break;
            }

            if (rect_hasPt(&box, pos.x, pos.y)) {
                // 确定文本内部偏移量
                *num = get_offset_in_text(txt, len, fid, pos.x - box.l, page->platform);
                return n;
            }

            *prev = n;
        }

        n = dll_nextNode(sel->rlst, n);
    }

    return N_NULL;
}

static nbool get_sel_head(NTextSel* sel, NPoint pos)
{
    nbool ret = N_FALSE;
    int num;
    NDLNode *prev, *next;
    NDLNode* n = find_selected_node(sel, pos, &num, &prev, &next);

    if (n == N_NULL) {
        n = next;
        num = 0;
    }

    if (n && (n != sel->h || num != sel->h_offs)) {
        ret = N_TRUE;
        sel->h = n;
        sel->h_offs = num;
        //fprintf(stderr, "head -> %d %d\n", ((NRenderNode*)dll_getData(n))->_no, num);
    }

    return ret;
}

static nbool get_sel_tail(NTextSel* sel, NPoint pos)
{
    nbool ret = N_FALSE;
    int num;
    NDLNode *prev, *next;
    NDLNode* n = find_selected_node(sel, pos, &num, &prev, &next);

    if (n == N_NULL) {
        n = prev;
        if (n) {
            NRect box;
            NFontId fid;
            get_text_info((NRenderNode*)dll_getData(n), &box, &fid, &num);
            num--;
        }
    }

    if (n && (n != sel->t || num != sel->t_offs)) {
        ret = N_TRUE;
        sel->t = n;
        sel->t_offs = num;
        //fprintf(stderr, "tail -> %d %d\n", ((NRenderNode*)dll_getData(n))->_no, num);
    }

    return ret;
}

// 填充相邻两选择区域之间的空隙
static void fill_sel_area_gap(NTextSel* sel)
{
    if (sel->sa[2].t < sel->sa[0].b) { // 水平位置关系
        sel->sa[1].l = sel->sa[0].r;
        sel->sa[1].t = N_MAX(sel->sa[0].t, sel->sa[2].t);
        sel->sa[1].r = sel->sa[2].l;
        sel->sa[1].b = N_MIN(sel->sa[0].b, sel->sa[2].b);
    }
    else { // 垂直位置关系
        sel->sa[1].l = N_MIN(sel->sa[0].l, sel->sa[2].l);
        sel->sa[1].t = sel->sa[0].b;
        sel->sa[1].r = N_MAX(sel->sa[0].r, sel->sa[2].r);
        sel->sa[1].b = sel->sa[2].t;
    }
}

// 产生选择区域矩形
static void adjust_sel_area(NTextSel* sel)
{
    NPage* page = (NPage*)sel->view->page;
    int i;
    //coord w;
    NRect box;
    NFontId fid;
    wchr* txt;
    int len;

    if (sel->h == N_NULL)
        return;

    if (sel->t == N_NULL) { // 初始状态
        get_text_info((NRenderNode*)dll_getData(sel->h), &box, &fid, &len);
        sel->t = sel->h;
        sel->t_offs = len - 1;
        if (!sel->useKey) {
            sel->h_offs = 0;
            sel->sa[0] = box;
        }
        rect_set(&sel->sa[1], 0, 0, 0, 0);
        rect_set(&sel->sa[2], 0, 0, 0, 0);
    }
    else if (sel->h == sel->t) { // 在同一结点内
        if (sel->h_offs > sel->t_offs) {
            i = sel->h_offs;
            sel->h_offs = sel->t_offs;
            sel->t_offs = i;
            if (sel->drag == NETS_DRAG_HEAD)
                sel->drag = NETS_DRAG_TAIL;
            else if (sel->drag == NETS_DRAG_TAIL)
                sel->drag = NETS_DRAG_HEAD;
        }
        txt = get_text_info((NRenderNode*)dll_getData(sel->h), &box, &fid, &len);
        sel->sa[0] = box;
        if (sel->h_offs > 0)
            sel->sa[0].l += NBK_gdi_getTextWidth(page->platform, fid, txt, sel->h_offs);
        if (sel->t_offs < len - 1)
            sel->sa[0].r = box.l + NBK_gdi_getTextWidth(page->platform, fid, txt, sel->t_offs + 1);
        rect_set(&sel->sa[1], 0, 0, 0, 0);
        rect_set(&sel->sa[2], 0, 0, 0, 0);
    }
    else { // 不同结点
        NDLNode* n;
        coord ml, mt, mr, mb;

        // 头尾交换
        if (((NRenderNode*)dll_getData(sel->h))->_no > ((NRenderNode*)dll_getData(sel->t))->_no) {
            n = sel->h;
            sel->h = sel->t;
            sel->t = n;
            i = sel->h_offs;
            sel->h_offs = sel->t_offs;
            sel->t_offs = i;
            if (sel->drag == NETS_DRAG_HEAD)
                sel->drag = NETS_DRAG_TAIL;
            else if (sel->drag == NETS_DRAG_TAIL)
                sel->drag = NETS_DRAG_HEAD;
        }

        txt = get_text_info((NRenderNode*)dll_getData(sel->h), &box, &fid, &len);
        sel->sa[0] = box;
        if (sel->h_offs > 0)
            sel->sa[0].l += NBK_gdi_getTextWidth(page->platform, fid, txt, sel->h_offs);

        txt = get_text_info((NRenderNode*)dll_getData(sel->t), &box, &fid, &len);
        sel->sa[2] = box;
        if (sel->t_offs < len - 1)
            sel->sa[2].r = box.l + NBK_gdi_getTextWidth(page->platform, fid, txt, sel->t_offs + 1);

        if (dll_nextNode(sel->rlst, sel->h) == sel->t) { // 相邻结点
            fill_sel_area_gap(sel);
            return;
        }

        sel->sa[1].l = sel->sa[1].t = N_MAX_COORD;
        sel->sa[1].r = sel->sa[1].b = 0;

        n = dll_nextNode(sel->rlst, sel->h);
        while (n && n != sel->t) {
            if (get_text_info((NRenderNode*)dll_getData(n), &box, &fid, &len)) {
                sel->sa[1].l = N_MIN(sel->sa[1].l, box.l);
                sel->sa[1].t = N_MIN(sel->sa[1].t, box.t);
                sel->sa[1].r = N_MAX(sel->sa[1].r, box.r);
                sel->sa[1].b = N_MAX(sel->sa[1].b, box.b);
            }
            n = dll_nextNode(sel->rlst, n);
        }
        if (sel->sa[1].l == N_MAX_COORD) {
            fill_sel_area_gap(sel);
            return;
        }

        //print_sel_area(sel);

        ml = N_MIN(sel->sa[2].l, sel->sa[1].l);
        mt = N_MIN(sel->sa[0].t, sel->sa[1].t);
        mr = N_MAX(sel->sa[0].r, sel->sa[1].r);
        mb = N_MAX(sel->sa[2].b, sel->sa[1].b);

        sel->sa[0].t = mt;
        sel->sa[0].r = mr;

        sel->sa[1].l = ml;
        sel->sa[1].t = sel->sa[0].b;
        sel->sa[1].r = mr;
        sel->sa[1].b = sel->sa[2].t;

        sel->sa[2].l = ml;
        sel->sa[2].b = mb;

        if (rect_getHeight(&sel->sa[1]) < 0)
            sel->sa[2].l = sel->sa[0].r;

        //print_sel_area(sel);
    }
}

// 计算选择标记区域
// indr [in]  1: 头标 0: 尾标
// indr [out] 1: 向下箭头 0: 向上箭头
static NRect calc_drag_mark_area(const NRect* box, nbool* indr, const NRect* va, const float* zoom)
{
    NRect r;
    NRect b = *box;

    if (zoom)
        rect_toView(&b, *zoom);

    if (*indr) { // 确定头标位置
        r.l = b.l - IA_AREA_GROW_X;
        r.r = b.l + IA_AREA_GROW_X;
        if (b.t - IA_AREA_GROW_Y >= va->t) {
            r.t = b.t - IA_AREA_GROW_Y;
            r.b = b.b;
        }
        else {
            r.t = b.t;
            r.b = b.b + IA_AREA_GROW_Y;
            *indr = N_FALSE;
        }
    }
    else { // 确定尾标位置
        r.l = b.r - IA_AREA_GROW_X;
        r.r = b.r + IA_AREA_GROW_X;
        if (b.b + IA_AREA_GROW_Y < va->b) {
            r.t = b.t;
            r.b = b.b + IA_AREA_GROW_Y;
        }
        else {
            r.t = b.t - IA_AREA_GROW_Y;
            r.b = b.b;
            *indr = N_TRUE;
        }
    }

    return r;
}

// 判断触点所在区域
static void decide_drag(NTextSel* sel, NPoint pos)
{
    if (sel->h == N_NULL || sel->t == N_NULL) {
        sel->drag = NETS_DRAG_HEAD;
        sel->dy = 0;
        //fprintf(stderr, "drag head\n");
    }
    else {
        NPage* page = (NPage*)sel->view->page;
        float zoom = history_getZoom((NHistory*)page->history);
        NRect box, r;
        NRect va;
        nbool indr;

        NBK_helper_getViewableRect(page->platform, &va);
        rect_toDoc(&va, zoom);

        pos.x = float_idiv(pos.x, zoom);
        pos.y = float_idiv(pos.y, zoom);

        // 检测头标响应区
        indr = N_TRUE;
        r = sel->sa[0];
        box = calc_drag_mark_area(&r, &indr, &va, N_NULL);
        if (rect_hasPt(&box, pos.x, pos.y)) {
            sel->drag = NETS_DRAG_HEAD;
            sel->dy = r.t + rect_getHeight(&r) / 2 - pos.y;
            sel->dy = float_imul(sel->dy, zoom);
            //fprintf(stderr, "drag head\n");
            return;
        }

        // 检测尾标响应区
        indr = N_FALSE;
        r = (sel->h == sel->t) ? sel->sa[0] : sel->sa[2];
        box = calc_drag_mark_area(&r, &indr, &va, N_NULL);
        if (rect_hasPt(&box, pos.x, pos.y)) {
            sel->drag = NETS_DRAG_TAIL;
            sel->dy = r.t + rect_getHeight(&r) / 2 - pos.y;
            sel->dy = float_imul(sel->dy, zoom);
            //fprintf(stderr, "drag tail\n");
            return;
        }

        sel->drag = NETS_DRAG_NONE;
        sel->dy = 0;
        //fprintf(stderr, "drag none\n");
    }
}

// 是否处于视图边缘
static nbool in_view_edge(NRect* va, NPoint* pos)
{
    nbool in = N_FALSE;
    NRect r;
    coord w = rect_getWidth(va);
    coord h = rect_getHeight(va);

    r = *va;
    r.b = r.t + SCREEN_EDGE; // 上边缘
    r.t -= h;
    if (rect_hasPt(&r, pos->x, pos->y))
        in = N_TRUE;

    if (!in) {
        r = *va;
        r.t = r.b - SCREEN_EDGE; // 下边缘
        r.b += h;
        if (rect_hasPt(&r, pos->x, pos->y))
            in = N_TRUE;
    }

    if (!in) {
        r = *va;
        r.r = r.l + SCREEN_EDGE; // 左边缘
        r.l -= w;
        if (rect_hasPt(&r, pos->x, pos->y))
            in = N_TRUE;
    }

    if (!in) {
        r = *va;
        r.l = r.r - SCREEN_EDGE; // 右边缘
        r.r += w;
        if (rect_hasPt(&r, pos->x, pos->y))
            in = N_TRUE;
    }

    return in;
}

// 移动视图
static nbool move_view(NPage* page, NRect* va, coord dx, coord dy, nbool reverse)
{
    NPoint pt;
    float zoom = history_getZoom((NHistory*)page->history);
    NBK_Event_RePosition evt;
    coord w = page_width(page);
    coord h = page_height(page);
    nbool moveLeft, moveRight, moveUp, moveDown;

    moveLeft    = (va->l > 0) ? N_TRUE : N_FALSE;
    moveRight   = (va->r < w) ? N_TRUE : N_FALSE;
    moveUp      = (va->t > 0) ? N_TRUE : N_FALSE;
    moveDown    = (va->b < h) ? N_TRUE : N_FALSE;

    pt.x = va->l;
    pt.y = va->t;

    if (reverse) { // 改变运动方向
        dx = -dx;
        dy = -dy;
    }

    if (dx < 0) {
        pt.x += N_ABS(dx);
        moveLeft = N_FALSE;
    }
    else {
        pt.x -= N_ABS(dx);
        moveRight = N_FALSE;
    }

    if (dy < 0) {
        pt.y += N_ABS(dy);
        moveUp = N_FALSE;
    }
    else {
        pt.y -= N_ABS(dy);
        moveDown = N_FALSE;
    }

    if (moveLeft || moveRight || moveUp || moveDown) {
        view_normalizePoint(page->view, &pt, *va, zoom);
        evt.x = pt.x;
        evt.y = pt.y;
        evt.zoom = zoom;
        nbk_cb_call(NBK_EVENT_REPOSITION, &page->cbEventNotify, &evt);
        return N_TRUE;
    }

    return N_FALSE;
}

// 拖拽选择标记
static void drag_mark(NTextSel* sel, NPoint pos, nbool decide)
{
    NPage* page = (NPage*)sel->view->page;
    nbool found = N_FALSE;
    nbool move = N_FALSE;
    NRect va;
    NPoint adjustPos;

    if (decide)
        decide_drag(sel, pos);

    adjustPos.x = pos.x;
    adjustPos.y = pos.y + sel->dy;

    NBK_helper_getViewableRect(page->platform, &va);

    if (sel->drag == NETS_DRAG_HEAD)
        found = get_sel_head(sel, adjustPos);
    else if (sel->drag == NETS_DRAG_TAIL)
        found = get_sel_tail(sel, adjustPos);

    if (found) {
        // 调整选择区域
        adjust_sel_area(sel);
    }

    if (sel->drag != NETS_DRAG_NONE) {
        // 检测触点是否在屏幕边缘
        move = in_view_edge(&va, &pos);
    }

    if (!decide && (sel->drag == NETS_DRAG_NONE || move)) {
        if (move_view(page, &va, pos.x - sel->lastPos.x, pos.y - sel->lastPos.y, move))
            return;
    }

    if (found) // 屏内拖动
        nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
}

// 移动标记
static void move_mark(NTextSel* sel, const nid key)
{
    NPage* page = (NPage*)sel->view->page;
    NDLNode** n;
    NRect box;
    NFontId fid;
    int len, *offs;
    wchr* txt;
    NPoint pt;
    NRect* sa;
    NRect va;
    coord dy = 0;

    NBK_helper_getViewableRect(page->platform, &va);

    if (sel->drag == NETS_DRAG_HEAD) {
        n = &sel->h;
        offs = &sel->h_offs;
        sa = &sel->sa[0];
    }
    else {
        n = &sel->t;
        offs = &sel->t_offs;
        sa = (sel->t == sel->h) ? &sel->sa[0] : &sel->sa[2];
    }

    txt = get_text_info((NRenderNode*)dll_getData(*n), &box, &fid, &len);
    point_set(&pt, sa->l, sa->t);
    if (*offs > 0)
        pt.x += NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);

    switch (key) {
    case NEKEY_ENTER:
        if (sel->moveH) {
            sel->moveH = N_FALSE;
            sel->drag = NETS_DRAG_TAIL;
        }
        else {
            nbk_cb_call(NBK_EVENT_TEXTSEL_FINISH, &page->cbEventNotify, N_NULL);
            return;
        }
        break;

    case NEKEY_LEFT:
        if (*offs > 0) {
            (*offs)--;
            sa->l = box.l;
            if (*offs > 0)
                sa->l += NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);
        }
        else {
            NDLNode* t = dll_prevNode(sel->rlst, *n);
            while (t) {
                txt = get_text_info((NRenderNode*)dll_getData(t), &box, &fid, &len);
                if (txt) {
                    *n = t;
                    *offs = len - 1;
                    *sa = box;
                    sa->l += NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);
                    break;
                }
                t = dll_prevNode(sel->rlst, t);
            }
        }
        break;

    case NEKEY_RIGHT:
        if (*offs < len - 1) {
            (*offs)++;
            sa->l = box.l + NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);
        }
        else {
            NDLNode* t = dll_nextNode(sel->rlst, *n);
            while (t) {
                txt = get_text_info((NRenderNode*)dll_getData(t), &box, &fid, &len);
                if (txt) {
                    *n = t;
                    *offs = 0;
                    *sa = box;
                    break;
                }
                t = dll_nextNode(sel->rlst, t);
            }
        }
        break;

    case NEKEY_UP:
    {
        NDLNode* t = dll_prevNode(sel->rlst, *n);
        while (t) {
            txt = get_text_info((NRenderNode*)dll_getData(t), &box, &fid, &len);
            if (txt && box.t < pt.y) {
                if (pt.x >= box.l && pt.x < box.r) {
                    *n = t;
                    *offs = get_offset_in_text(txt, len, fid, pt.x - box.l, page->platform);
                    *sa = box;
                    if (*offs > 0)
                        sa->l += NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);
                    break;
                }
                else if (pt.x >= box.r) {
                    *n = t;
                    *offs = len - 1;
                    *sa = box;
                    sa->l += NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);
                    break;
                }
            }
            t = dll_prevNode(sel->rlst, t);
        }
        if (txt) {
            point_set(&pt, sa->l, sa->t);
            dy = -(rect_getHeight(&va) / 2);
        }
        break;
    }

    case NEKEY_DOWN:
    {
        NDLNode* t = dll_nextNode(sel->rlst, *n);
        txt = N_NULL;
        while (t) {
            txt = get_text_info((NRenderNode*)dll_getData(t), &box, &fid, &len);
            if (txt && box.t > pt.y) {
                if (pt.x >= box.l && pt.x < box.r) {
                    *n = t;
                    *offs = get_offset_in_text(txt, len, fid, pt.x - box.l, page->platform);
                    *sa = box;
                    if (*offs > 0)
                        sa->l += NBK_gdi_getTextWidth(page->platform, fid, txt, *offs);
                    break;
                }
                else if (pt.x < box.l) {
                    *n = t;
                    *offs = 0;
                    *sa = box;
                    break;
                }
            }
            t = dll_nextNode(sel->rlst, t);
        }
        if (txt) {
            point_set(&pt, sa->l, sa->b);
            dy = rect_getHeight(&va) / 2;
        }
        break;
    }

    default:
        break;
    }

    if (!sel->moveH)
        adjust_sel_area(sel);

    if (dy) {
        if (in_view_edge(&va, &pt))
            if (move_view(page, &va, 0, dy, N_TRUE))
                return;
    }

    nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
}

static int count_selected_words(NTextSel* sel)
{
    int count = 0;
    NDLNode* n;
    NRect box;
    NFontId fid;
    int len;

    if (sel->h == N_NULL)
        return 0;

    if (sel->h == sel->t)
        return sel->t_offs - sel->h_offs + 1;

    if (get_text_info((NRenderNode*)dll_getData(sel->h), &box, &fid, &len))
        count += len - sel->h_offs;

    count += sel->t_offs + 1;

    n = dll_nextNode(sel->rlst, sel->h);
    while (n && n != sel->t) {
        if (get_text_info((NRenderNode*)dll_getData(n), &box, &fid, &len))
            count += len;
        n = dll_nextNode(sel->rlst, n);
    }

    return count;
}

NTextSel* textSel_begin(NView* view)
{
    NTextSel* ts = (NTextSel*)NBK_malloc0(sizeof(NTextSel));

    ts->view = view;
    ts->rlst = dll_create();

    return ts;
}

void textSel_end(NTextSel** sel)
{
    NTextSel* ts = *sel;

    dll_delete(&ts->rlst);
    NBK_free(ts);

    *sel = N_NULL;
}

void textSel_setStartPoint(NTextSel* sel, const NPoint* point)
{
    NPage* page = (NPage*)sel->view->page;
    NPoint pt = *point;

    if (!sel->init)
        build_render_list(sel);

    if (get_sel_head(sel, pt)) {
        sel->drag = NETS_DRAG_HEAD;
        adjust_sel_area(sel);
        nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
    }
}

void textSel_useKey(NTextSel* sel, const NPoint* point)
{
    NPage* page = (NPage*)sel->view->page;
    NPoint pt = *point;
    NRect box;
    NFontId fid;
    int len;
    wchr* txt;

    if (!sel->init)
        build_render_list(sel);

    sel->useKey = N_TRUE;

    // 选择初始结点，不允许为空
    get_sel_head(sel, pt);
    if (sel->h == N_NULL)
        sel->h = dll_firstNode(sel->rlst);
    N_ASSERT(sel->h);
    sel->drag = NETS_DRAG_HEAD;
    sel->moveH = N_TRUE;

    txt = get_text_info((NRenderNode*)dll_getData(sel->h), &box, &fid, &len);
    sel->sa[0] = box;
    if (sel->h_offs > 0)
        sel->sa[0].l += NBK_gdi_getTextWidth(page->platform, fid, txt, sel->h_offs);

    nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
}

nbool textSel_processEvent(NTextSel* sel, NEvent* evt)
{
    nbool handled = N_TRUE;

    if (evt->type == NEEVENT_PEN) {
        // 触摸操作
        switch (evt->d.penEvent.type) {
        case NEPEN_DOWN:
            sel->lastPos = evt->d.penEvent.pos;
            if (!sel->init)
                build_render_list(sel);
            drag_mark(sel, evt->d.penEvent.pos, N_TRUE);
            break;

        case NEPEN_MOVE:
            if (   N_ABS((evt->d.penEvent.pos.x - sel->lastPos.x)) >= MIN_STEP
                || N_ABS((evt->d.penEvent.pos.y - sel->lastPos.y)) >= MIN_STEP ) {
                drag_mark(sel, evt->d.penEvent.pos, N_FALSE);
                sel->lastPos = evt->d.penEvent.pos;
            }
            break;

        case NEPEN_UP:
            break;

        default:
            handled = N_FALSE;
            break;
        }
    }
    else if (evt->type == NEEVENT_KEY) {
        // 按键操作
        switch (evt->d.keyEvent.key) {
        case NEKEY_UP:
        case NEKEY_DOWN:
        case NEKEY_LEFT:
        case NEKEY_RIGHT:
        case NEKEY_ENTER:
            move_mark(sel, evt->d.keyEvent.key);
            break;
        default:
            handled = N_FALSE;
            break;
        }
    }
    else
        handled = N_FALSE;

    return handled;
}

static void paint_with_head_pos(NTextSel* sel, NRect* rect)
{
    NPage* page = (NPage*)sel->view->page;
    float zoom = history_getZoom((NHistory*)page->history);
    NRect pr, cl;
    nbool indr;
    NRect vr;

    cl.l = cl.t = 0;
    cl.r = rect_getWidth(rect);
    cl.b = rect_getHeight(rect);
    NBK_gdi_setClippingRect(page->platform, &cl);

    indr = N_TRUE;
    pr = calc_drag_mark_area(&sel->sa[0], &indr, &vr, &zoom);
    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
    NBK_drawStockImage(page->platform, ((indr) ? RES_TSEL_DOWN : RES_TSEL_UP), &pr, NECS_NORMAL, &cl);

    NBK_gdi_cancelClippingRect(page->platform);
}

static void paint_with_head_and_tail(NTextSel* sel, NRect* rect)
{
    NPage* page = (NPage*)sel->view->page;
    float zoom = history_getZoom((NHistory*)page->history);
    NRect pr, cl;
    int i;
    NColor color;
    coord x, y, w, h;
    NRect vr;
    nbool indr;
    int num;
    wchr wc[8];
    int len;

    color = colorBlue; color.a = 64; // 选择区背景色

    cl.l = cl.t = 0;
    cl.r = rect_getWidth(rect);
    cl.b = rect_getHeight(rect);
    NBK_gdi_setClippingRect(page->platform, &cl);

    // 绘制选择区
    for (i=0; i < TEXT_SEL_AREA_NUM; i++) {
        pr = sel->sa[i];
        if (rect_getWidth(&pr) > 0 && rect_getHeight(&pr) > 0) {
            rect_toView(&pr, zoom);
            rect_move(&pr, pr.l - rect->l, pr.t - rect->t);
            NBK_gdi_setBrushColor(page->platform, &color);
            NBK_gdi_fillRect(page->platform, &pr);
        }
    }

    NBK_helper_getViewableRect(page->platform, &vr);

    if (sel->fid == 0) {
        int fsize = (page->settings) ? page->settings->mainFontSize : 0;
        if (fsize == 0) fsize = DEFAULT_FONT_SIZE;
        sel->fid = NBK_gdi_getFont(page->platform, fsize, N_FALSE, N_FALSE);
    }

    // 统计选择的字符
    num = count_selected_words(sel);
    w = h = 0;
    if (num > 0) {
        char mb[8];
        len = sprintf(mb, "%d", num);
        for (i=0; i < len; i++) // utf-8 -> utf-16
            wc[i] = mb[i];

        w = NBK_gdi_getTextWidth(page->platform, sel->fid, wc, len);
        h = NBK_gdi_getFontHeight(page->platform, sel->fid);
        w += COUNT_DA * 2;
        h += COUNT_DA * 2;
    }

    // 绘制头部标记
    indr = N_TRUE;
    pr = calc_drag_mark_area(&sel->sa[0], &indr, &vr, &zoom);

    if (sel->drag == NETS_DRAG_HEAD) {
        sel->countPt.x = pr.l + rect_getWidth(&pr) / 2;
        sel->countPt.y = (indr) ? pr.t : pr.b - h;
    }

    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    NBK_drawStockImage(page->platform, ((indr) ? RES_TSEL_DOWN : RES_TSEL_UP), &pr, NECS_NORMAL, &cl);

    // 绘制尾部标记
    indr = N_FALSE;
    pr = calc_drag_mark_area((sel->h == sel->t) ? &sel->sa[0] : &sel->sa[2], &indr, &vr, &zoom);

    if (sel->drag == NETS_DRAG_TAIL) {
        sel->countPt.x = pr.l + rect_getWidth(&pr) / 2;
        sel->countPt.y = (indr) ? pr.t : pr.b - h;
    }

    rect_move(&pr, pr.l - rect->l, pr.t - rect->t);

    NBK_drawStockImage(page->platform, ((indr) ? RES_TSEL_DOWN : RES_TSEL_UP), &pr, NECS_NORMAL, &cl);

    // 绘制字数统计
    if (num > 0) {
        NPoint pt;

        if (sel->countPt.x + IA_AREA_GROW_X + COUNT_DA + w < vr.r)
            x = sel->countPt.x + IA_AREA_GROW_X + COUNT_DA;
        else
            x = sel->countPt.x - IA_AREA_GROW_X - w + COUNT_DA;
        
        y = sel->countPt.y + COUNT_DA;

        rect_set(&pr, x - rect->l - COUNT_DA, y - rect->t - COUNT_DA, w, h);
        NBK_gdi_setBrushColor(page->platform, &colorGray);
        NBK_gdi_fillRect(page->platform, &pr);

        pt.x = x - rect->l;
        pt.y = y - rect->t;
        NBK_gdi_setPenColor(page->platform, &colorWhite);
        NBK_gdi_useFontNoZoom(page->platform, sel->fid);
        NBK_gdi_drawText(page->platform, wc, len, &pt, 0);
        NBK_gdi_releaseFont(page->platform);
    }

    NBK_gdi_cancelClippingRect(page->platform);
}

void textSel_paint(NTextSel* sel, NRect* rect)
{
    if (sel->h == N_NULL)
        return;

    if (sel->useKey && sel->t == N_NULL)
        paint_with_head_pos(sel, rect);
    else
        paint_with_head_and_tail(sel, rect);
}

static int get_selected_text(NTextSel* sel, wchr** buf)
{
    int num = 0;
    NDLNode* n = sel->h;
    NRenderNode* r;
    wchr* txt;
    NRect box;
    NFontId fid;
    int len, offs, size;
    wchr* p = (buf) ? *buf : N_NULL;

    while (n) {
        r = (NRenderNode*)dll_getData(n);

        txt = get_text_info(r, &box, &fid, &len);
        if (txt) {
            offs = 0;
            size = len;
            if (n == sel->h) {
                offs = sel->h_offs;
                size -= offs;
            }
            if (n == sel->t) {
                size -= len - (sel->t_offs + 1);
            }
            num += size;
            if (p) {
                nbk_wcsncpy(p, txt + offs, size);
                p += size;
            }
        }
        else { // 插入换行
            num += 2;
            if (p) {
                *p++ = 0xd;
                *p++ = 0xa;
            }
        }

        if (n == sel->t)
            break;
        n = dll_nextNode(sel->rlst, n);
    }

    return num;
}

wchr* textSel_getText(NTextSel* sel, int* len)
{
    wchr* text = N_NULL;
    int num = get_selected_text(sel, N_NULL);
    if (num > 0) {
        text = (wchr*)NBK_malloc(sizeof(wchr) * (num + 1));
        get_selected_text(sel, &text);
        *len = num;
    }
    return text;
}
