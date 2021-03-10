/*
 * page.c
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_callback.h"
#include "../css/color.h"
#include "../css/css_val.h"
#include "page.h"
#include "node.h"
#include "attr.h"
#include "token.h"
#include "xml_helper.h"
#include "xml_tags.h"
#include "xml_atts.h"
#include "history.h"
#include "../loader/loader.h"
#include "../loader/url.h"
#include "../tools/str.h"
#include "../tools/dump.h"
#include "../tools/slist.h"
#include "../render/imagePlayer.h"
#include "../render/renderNode.h"
#include "../render/renderInput.h"
#include "../render/renderTextarea.h"

#define DEBUG_ATTACH        0
#define DEBUG_INC_OP        0
#define TEST_PAINT_CONSUME  0

void* g_dp = N_NULL; // for debug output

static char* l_defaultStyle = "input,select,textarea{margin:2;}";

#ifdef NBK_MEM_TEST
int page_memUsed(const NPage* page)
{
    int size = 0;
    if (page) {
        size = sizeof(NPage);
        size += doc_memUsed(page->doc);
        size += view_memUsed(page->view);
        if (page->main)
            size += sheet_memUsed(page->sheet);
        if (page->main && page->subPages) {
            int c = 0;
            NPage* p = sll_first(page->subPages);
            while (p) {
                c++;
                size += page_memUsed(p);
                p = sll_next(page->subPages);
            }
            
            dump_char((void*)page, "sub-page count:", -1);
            dump_int((void*)page, c);
            dump_return((void*)page);
        }
    }
    return size;
}

void mem_checker(const NPage* page)
{
    if (page) {
        int size = page_memUsed(page);
        
        dump_char((void*)page, "memory total:", -1);
        dump_int((void*)page, size);
        dump_return((void*)page);
        dump_flush((void*)page);
    }
}

#endif

int page_testApi(NPage* page)
{
    return 0;
}

NPage* page_create(NPage* parent, void* pfd)
{
    NPage* p = (NPage*)NBK_malloc0(sizeof(NPage));
    N_ASSERT(p);
    
    p->main = (parent) ? 0 : 1;
    p->parent = parent;
    p->begin = 1;
    p->attach = (parent) ? 0 : 1;
    if (p->main)
        p->sheet = sheet_create(p);
	p->platform = pfd;
    p->doc = doc_create(p);
    p->view = view_create(p);
    
    return p;
}

static void page_reset(NPage* page)
{
    page->rece = 0;

    if (page->subPages) {
        NPage* p = (NPage*)sll_first(page->subPages);
        while (p) {
            page_delete(&p);
            sll_removeCurr(page->subPages);
            p = (NPage*)sll_next(page->subPages);
        }
        sll_delete(&page->subPages);
    }
}

void page_delete(NPage** page)
{
    NPage* p = *page;

    if (p->main) {
        sheet_delete(&p->sheet);
        if (p->incPage)
            page_delete(&p->incPage);
    }
    view_delete(&p->view);
	smartptr_release(p->doc);

    page_reset(p);

	NBK_free(p);
    *page = N_NULL;
}

void page_setScreenWidth(NPage* page, coord width)
{
    page->view->scrWidth = width;
}

NUpCmdSet page_decideRequestMode(NPage* page, const char* url, nbool submit)
{
    NUpCmdSet settings;
    
    settings.image = page->settings->allowImage;
    settings.fixedScreen = page->settings->fixedScreen;

	if (url && nbk_strfind(url, "file:/") == 0) {
        settings.via = NEREV_STANDARD;
    }
    else if (page->settings->support != NESupport) {
        if (page->settings->support == NENotSupportByFlyflow)
            settings.via = NEREV_STANDARD;
        else
            settings.via = NEREV_TF;
        page->settings->support = NESupport;
    }
    else if (doc_getType(page->doc) == NEDOC_WML) {
        settings.via = page->mode;
    }
    else if (submit) {
        if (!history_getLastRequestMode((NHistory*)page->history, &settings.via))
            settings.via = page->mode;
    }
    else if (page->settings->initMode != NEREV_STANDARD) {
        settings.via = page->settings->mode;
    }
    else {
        if (page->settings->initMode == NEREV_STANDARD)
            settings.via = NEREV_STANDARD;
        else
            settings.via = page->settings->mode;
        
        // fixme: 请求可能是图片，采用直连
        if (url) {
            int len = nbk_strlen(url);
            if (len > 4) {
                char* ext = (char*)url + len - 4;
                if (nbk_strfind(".jpg.png.gif", ext) != -1)
                    settings.via = NEREV_STANDARD;
            }
        }
    }

    history_setRequestMode((NHistory*)page->history, settings.via);
    
	//dump_char(page, "decide access type", -1);
	//dump_char(page, (char*)url, -1);
	//dump_int(page, settings.via);
	//dump_return(page);
    
    page->docNum = 0;
    page->cache = 0;
    
    return settings;
}

void page_loadData(NPage* page, const char* url, const char* str, int length)
{
    loader_stopAll(page->platform, page_getStopId(page));
    view_stop(page->view);
    doc_stop(page->doc);
    nbk_cb_call(NBK_EVENT_NEW_DOC_BEGIN, &page->cbEventNotify, N_NULL);
    
    page_reset(page);
    if (page->doc->root) {
        history_add((NHistory*)page->history);
    }
    sheet_reset(page->sheet);
    doc_begin(page->doc);
    doc_setUrl(page->doc, url);
    doc_write(page->doc, str, length, N_TRUE);
    doc_end(page->doc);
    
//    sheet_dump(page->sheet);
}

void page_loadUrlStandard(NPage* page, const char* url, nid enc, char* body, NFileUpload** files)
{
    NRequest* req;
    char *dstUrl, *referer = N_NULL;
    
    loader_stopAll(page->platform, page_getStopId(page));
    view_stop(page->view);
    doc_stop(page->doc);
    nbk_cb_call(NBK_EVENT_NEW_DOC, &page->cbEventNotify, N_NULL);
    
	dstUrl = str_clone(url);
	if (page->doc->url && nbk_strlen(page->doc->url) > 0)
		referer = str_clone(page->doc->url);
    
    page->begin = 1;
    page->encoding = NEENC_UNKNOWN;

    req = loader_createRequest();
    req->enc = (uint8)enc;
    req->body = body;
    if (page->settings->via == NEREV_TF) {
        page->settings->via = NEREV_STANDARD;
        req->via = NEREV_TF;
    }
    if (files) {
        req->files = *files; *files = N_NULL;
    }
    if (page->refresh) {
        req->pageId = page->id;
        req->method = NEREM_REFRESH;
    }
    else
        req->pageId = (page->doc->root) ? history_getPreId((NHistory*)page->history) : page->id;

    loader_setRequest(req, NEREQT_MAINDOC, N_NULL, page);
    loader_setRequestUrl(req, dstUrl, N_TRUE);
    if (referer) loader_setRequestReferer(req, referer, N_TRUE);
    req->responseCallback = page_onResponse;
    req->platform = page->platform;

    if (!loader_load(req))
        smartptr_release(req);
}

void page_loadUrl(NPage* page, const char* url)
{
    char* u = url_parse(N_NULL, url);
    if (u) {
        page_loadUrlInternal(page, u, 0, N_NULL, N_NULL, N_FALSE);
        NBK_free(u);
    }
}

void page_loadUrlInternal(NPage* page, const char* url, nid enc, char* body, NFileUpload** files, nbool submit)
{
    NUpCmdSet settings = page_decideRequestMode(page, url, submit);
    
    if (settings.via == NEREV_STANDARD) {
        page_loadUrlStandard(page, url, enc, body, files);
    }
}

void page_loadFromCache(NPage* page)
{
    NRequest* req;
	char* url = history_getUrl((NHistory*)page->history, page->id);
    
    view_stop(page->view);
    doc_stop(page->doc);
    nbk_cb_call(NBK_EVENT_HISTORY_DOC, &page->cbEventNotify, N_NULL);

    page->begin = 1;
    page->encoding = history_getCharset((NHistory*)page->history);
    page->cache = 1;
    page->docNum = 0;

    req = loader_createRequest();
    loader_setRequest(req, NEREQT_MAINDOC, url, page);
    req->method = NEREM_HISTORY;
    req->pageId = page->id;
    req->responseCallback = page_onResponse;
    req->platform = page->platform;

    if (!loader_load(req))
        smartptr_release(req);
}

void page_refresh(NPage* page)
{
    char* url = history_getUrl((NHistory*)page->history, page->id);
    if (page->main && url) {
        page->refresh = 1;
        page_loadUrl(page, url);
    }
}

coord page_width(NPage* page)
{
    float zoom = history_getZoom((NHistory*)page->history);
    coord width = (coord)float_imul(view_width(page->view), zoom);
    return width;
}

coord page_height(NPage* page)
{
    float zoom = history_getZoom((NHistory*)page->history);
    coord height = (coord)float_imul(view_height(page->view), zoom);
    NRect va;
    NBK_helper_getViewableRect(page->platform, &va);
    if (height < rect_getHeight(&va))
        height = rect_getHeight(&va);
    return height;
}

nbool page_4ways(NPage* page)
{
    return (page->doc->type == NEDOC_FULL /*|| page->doc->type == NEDOC_NARROW*/);
}

void page_paint(NPage* page, NRect* rect)
{
#if TEST_PAINT_CONSUME
    uint32 t_time = NBK_currentMilliSeconds();
#endif
    NRect r;

    if (   page->workState != NEWORK_IDLE
        || page->view->root == N_NULL
        || page->view->root->needLayout )
        return;
    
    // initialize background
    r.l = r.t = 0;
    r.r = rect_getWidth(rect);
    r.b = rect_getHeight(rect);
    if (page->settings && page->settings->night)
        NBK_gdi_setBrushColor(page->platform, &page->settings->night->background);
    else
        NBK_gdi_setBrushColor(page->platform, &colorWhite);
	NBK_gdi_setClippingRect(page->platform, &r);
    NBK_gdi_fillRect(page->platform, &r);
	NBK_gdi_cancelClippingRect(page->platform);
    
    if (view_isZindex(page->view)) {
        view_paintWithZindex(page->view, rect);
        view_paintFocusWithZindex(page->view, rect);
    }
    else {
        view_paint(page->view, rect);
        if (page->subPages) {
            NPage* p = (NPage*)sll_first(page->subPages);
            while (p) {
                if (p->attach)
                    view_paint(p->view, rect);
                p = (NPage*)sll_next(page->subPages);
            }
        }
        view_paintFocus(page->view, rect);
    }
    
    view_paintControl(page->view, rect); // fixme: z-index?
    
#if TEST_PAINT_CONSUME
    if (page->doc->finish) {
        dump_char(page, "page paint", -1);
        dump_uint(page, NBK_currentMilliSeconds() - t_time);
        dump_NRect(page, rect);
        dump_return(page);
    }
#endif
}

void page_paintControl(NPage* page, NRect* rect)
{
    view_paintControl(page->view, rect);
}

void page_setEventNotify(NPage* page, NBK_CALLBACK func, void* user)
{
    g_dp = page;
    page->cbEventNotify.func = func;
    page->cbEventNotify.user = user;
}

//#define PAGE_TEST_RELEASE
static void page_begin(NPage* page, const char* url)
{
#ifdef PAGE_TEST_RELEASE
    uint32 t_consume;
#endif
    
    if (!page->begin)
        return;
    
#ifdef PAGE_TEST_RELEASE
    t_consume = NBK_currentMilliSeconds();
    dump_char(page, "page clear ===", -1);
    dump_return(page);
#endif
    
    page->begin = 0;
    page->incNum = 0;
    
    page_setFocusedNode(page, N_NULL);
    page_reset(page);

    if (page->main) {
        NHistory* history = (NHistory*)page->history;
        
        if (!page->cache && !page->refresh) {
            if (page->doc->root)
                history_add(history);
			history_setUrl(history, url);
		}
        if (page->refresh) {
            history_setZoom(history, 1.0f);
        }

        sheet_reset(page->sheet);

        {
            // 载入默认样式
            int len = nbk_strlen(l_defaultStyle);
            char* style = str_clone(l_defaultStyle);
            sheet_parseStyle(page->sheet, style, len);
            NBK_free(style);
        }
    }
    doc_begin(page->doc);
    if (url)
        doc_setUrl(page->doc, url);
    
    if (page->wbxml) {
        page->wbxml = 0;
        doc_attachWbxmlDecoder(page->doc, page->total);
    }
    page->doc->encoding = page->encoding;
    
    nbk_cb_call(NBK_EVENT_NEW_DOC_BEGIN, &page->cbEventNotify, N_NULL);
    //page->cache = 0;
    page->refresh = 0;
    
#ifdef PAGE_TEST_RELEASE
    dump_char(page, "page clear", -1);
    dump_uint(page, NBK_currentMilliSeconds() - t_consume);
    dump_return(page);
#endif
}

static nbool attach_page(NPage* main, NPage* sub, const char* name)
{
    NSList* lst;
    nid tags[] = {TAGID_FRAME, 0};
    char* tc_name;
    
    lst = node_getListByTag(main->doc->root, tags);
    if (lst) {
        NNode* n = (NNode*)sll_first(lst);
        while (n) {
            tc_name = attr_getValueStr(n->atts, ATTID_TC_NAME);
#if DEBUG_ATTACH
            {
                NPage* p = page_getMainPage(sub);
                dump_char(p, tc_name, -1);
                dump_return(p);
            }
#endif
            
            if (tc_name && !nbk_strcmp(tc_name, name)) {
                NRect* tc_rect = attr_getRect(n->atts, ATTID_TC_RECT);
                NNode* body = node_getByTag(main->doc->root, TAGID_BODY);
                NRect* body_rect = attr_getRect(body->atts, ATTID_TC_RECT);
                sub->view->frameRect = *tc_rect;
                rect_move(&sub->view->frameRect,
                    sub->view->frameRect.l + body_rect->l,
                    sub->view->frameRect.t + body_rect->t);
                n->child = sub->doc->root;
                sub->doc->root->parent = n; // 子文档加入父文档引用
                sub->attach = 1;
            }
            
            if (sub->attach)
                break;

            n = (NNode*)sll_next(lst);
        }
        sll_delete(&lst);
    }
    
    return (nbool)sub->attach;
}

static void page_attach_subpage(NPage* main, NPage* sub)
{
    char* name;
    
    if (main == sub)
        return;
    
    name = doc_getFrameName(sub->doc);
    if (name == N_NULL)
        return;
    
#if DEBUG_ATTACH
    dump_char(main, "attach", -1);
    dump_char(main, name, -1);
#endif
    
    if (attach_page(main, sub, name))
        ;
    else if (main->subPages) {
        NPage* p = (NPage*)sll_first(main->subPages);
        while (p) {
            if (attach_page(p, sub, name))
                break;
            p = (NPage*)sll_next(main->subPages);
        }
    }
    
#if DEBUG_ATTACH
    dump_int(main, sub->attach);
    dump_return(main);
#endif
}

static NPage* page_new_subpage(NPage* page)
{
    NPage* sub;
    
    if (page->subPages == N_NULL)
        page->subPages = sll_create();
    
	sub = page_create(page, page->platform);
    sub->sheet = page->sheet;
    sub->history = page->history;
    sub->settings = page->settings;
    
    sll_append(page->subPages, sub);
    
    return sub;
}

void page_onResponse(NEResponseType type, void* user, void* data, int v1, int v2)
{
    NRequest* req = (NRequest*)user;
    NPage* page = (NPage*)req->user;
    
    switch (type) {
    case NEREST_HEADER:
    {
        NRespHeader* hdr = (NRespHeader*)data;
        if (hdr->mime == NEMIME_DOC_WMLC)
            page->wbxml = 1;
        if (!page->cache)
            page->encoding = hdr->encoding;
        page->total = hdr->content_length;
        nbk_cb_call(NBK_EVENT_GOT_RESPONSE, &page->cbEventNotify, N_NULL);
        break;
    }
    
    case NEREST_DATA:
    {
        NBK_Event_GotResponse evtData;
        page_begin(page, req->url);
        page->rece += v2;
        doc_write(page->doc, (char*)data, v1, N_FALSE);
        evtData.received = page->rece;
        evtData.total = page->total;
        nbk_cb_call(NBK_EVENT_GOT_DATA, &page->cbEventNotify, &evtData);
        break;
    }
        
    case NEREST_COMPLETE:
    {
        doc_end(page->doc);
        history_restoreFormData((NHistory*)page->history, page);
        //sheet_dump(page->sheet);
        break;
    }
        
    case NEREST_ERROR:
    {
        doc_end(page->doc);
		if (!NBK_handleError(v1)) {
			char* errorPage = NBK_getStockPage((v1 == 404) ? NESPAGE_ERROR_404 : NESPAGE_ERROR);
			doc_doGet(page->doc, errorPage, N_FALSE, N_FALSE);
			nbk_cb_call(NBK_EVENT_LOADING_ERROR_PAGE, &page->cbEventNotify, N_NULL);
		}
        break;
    }
    
    case NEREST_CANCEL:
        doc_end(page->doc);
        break;

    } // switch
}

void page_stop(NPage* page)
{
    page->refresh = 0;
    loader_stopAll(page->platform, page_getStopId(page));
    view_stop(page->view);
}

void page_enablePic(NPage* page, nbool enable)
{
    if (page->view)
        view_enablePic(page->view, enable);
}

void page_setFocusedNode(NPage* page, NNode* node)
{
    if (page->main == N_FALSE)
        return;

    // if last node focused is in editing, changes the state.
    node_cancelEditing(page->view->focus, page);
    page->view->focus = node;
    
    if (page->subPages) {
        NPage* p = (NPage*)sll_first(page->subPages);
        while (p) {
            p->view->focus = node;
            p = (NPage*)sll_next(page->subPages);
        }
    }
}

//nbool page_hasMainBody(NPage* page)
//{
//    nbool has = N_FALSE;
//    
//    if (page->main) {
//        if (page->doc->mbList)
//            has = N_TRUE;
//    }
//    
//    return has;
//}
//
//typedef struct _NMainBodyCreateTask {
//    NPage*  page;
//    nbool    ignore;
//} NMainBodyCreateTask;
//
//static int create_main_body_tag_start(NNode* node, void* user, nbool* ignore)
//{
//    NMainBodyCreateTask* task = (NMainBodyCreateTask*)user;
//    NToken token;
//    char* cls;
//    
//    if (task->ignore)
//        return 0;
//    else if (node->id == TAGID_FORM || node->id == TAGID_FRAME) {
//        task->ignore = N_TRUE;
//        return 0;
//    }
//    
//    NBK_memset(&token, 0, sizeof(NToken));
//    token.id = node->id;
//    token.nodeRef = node;
//
//    cls = attr_getValueStr(node->atts, ATTID_CLASS);
//    if (cls && !nbk_strcmp(cls, "tc_title")) {
//        token.atts = (NAttr*)NBK_malloc0(sizeof(NAttr) * 2);
//        token.atts[0].id = ATTID_CLASS;
//        token.atts[0].d.ps = cls;
//    }
//
//    if (node->id == TAGID_TEXT && node->len > 0) {
//        token.len = node->len;
//        token.d.text = node->d.text;
//    }
//    
//    if (node->id != TAGID_TEXT || (node->id == TAGID_TEXT && node->len > 0))
//        doc_processToken(task->page->doc, &token);
//    
//    return 0;
//}
//
//static int create_main_body_tag_end(NNode* node, void* user)
//{
//    NMainBodyCreateTask* task = (NMainBodyCreateTask*)user;
//    NToken token;
//    
//    if (task->ignore) {
//        if (node->id == TAGID_FORM || node->id == TAGID_FRAME)
//            task->ignore = N_FALSE;
//        return 0;
//    }
//    
//    NBK_memset(&token, 0, sizeof(NToken));
//    token.id = node->id + TAGID_CLOSETAG;
//    
//    doc_processToken(task->page->doc, &token);
//    
//    return 0;
//}
//
//void page_createMainBody(NPage* page, NNode* root, NPage* mainBody)
//{
//    NMainBodyCreateTask task;
//    
////    doc_dumpDOM(page->doc);
//
//    page_resetMainBody(mainBody);
//    
//    mainBody->doc->type = NEDOC_MAINBODY;
//    mainBody->id = page->id;
//    mainBody->view->imgPlayer->pageId = page->id;
//    mainBody->cache = 1;
//    mainBody->cbEventNotify.func = N_NULL;
//    mainBody->doc->syncMode = 1;
//    mainBody->history = page->history;
//    
//    task.page = mainBody;
//    task.ignore = N_FALSE;
//    node_traverse_depth(root,
//        create_main_body_tag_start,
//        create_main_body_tag_end,
//        &task);
//    
//    mainBody->doc->syncMode = 0;
//    mainBody->doc->finish = 0;
//    mainBody->cbEventNotify = page->cbEventNotify;
//    doc_end(mainBody->doc);
//    
////    doc_dumpDOM(mainBody->doc);
//}
//
//void page_resetMainBody(NPage* mainBody)
//{
//    char* style = \
//        "div{padding:0px 5px 10px;}" \
//        ".tc_title{font-weight:bold;text-align:center;padding:10px;}" \
//        "*{line-height:150%;color:#333;background:#efefef;}";
//    
//    page_reset(mainBody);
//    doc_begin(mainBody->doc);
//    if (   mainBody->settings
//        && mainBody->settings->mainBodyStyle) {
//        sheet_parseStyle(mainBody->sheet, mainBody->settings->mainBodyStyle, -1);
//    }
//    else {
//        int len = nbk_strlen(style);
//        char* data = (char*)NBK_malloc(len + 1);
//        nbk_strcpy(data, style);
//        sheet_parseStyle(mainBody->sheet, data, len);
//        NBK_free(data);
//    }
//}
//
//NNode* page_getMainBodyByPos(NPage* page, coord x, coord y)
//{
//    if (page_hasMainBody(page)) {
//        NNode* n = (NNode*)sll_first(page->doc->mbList);
//        while (n) {
//            NRenderNode* rn = (NRenderNode*)n->render;
//            if (rn && rect_hasPt(&rn->r, x, y))
//                return n;
//            n = (NNode*)sll_next(page->doc->mbList);
//        }
//    }
//    return N_NULL;
//}
//
//nbool page_isPaintMainBodyBorder(NPage* page, NRect* rect)
//{
//    nbool ret = N_FALSE;
//    rect_toDoc(rect, history_getZoom((NHistory*)page->history));
//    
//    if (page_hasMainBody(page)) {
//        NNode* n = (NNode*)sll_first(page->doc->mbList);
//        while (n) {
//            NRenderNode* rn = (NRenderNode*)n->render;
//            if (rn && (rect_getWidth(&rn->r) < rect_getWidth(rect) || rect_getHeight(&rn->r) < rect_getHeight(rect))) {
//                ret = N_TRUE;
//                break;
//            }
//            n = (NNode*)sll_next(page->doc->mbList);
//        }
//    }
//    
//    return ret;
//}
//
//nbool page_isMainBodyMode(NPage* page)
//{
//    nbool ret = N_FALSE;
//    
//    if (page->doc->type == NEDOC_FULL) {
//        NPage* p = history_curr((NHistory*)page->history);
//        if (p->doc->type == NEDOC_MAINBODY)
//            ret = N_TRUE;
//    }
//    
//    return ret;
//}

void page_setId(NPage* page, nid id)
{
    page->id = id;
    page->view->imgPlayer->pageId = id;
}

nid page_getId(NPage* page)
{
	return page->id;
}

//void page_updateCtrlNode(NPage* page, NNode* node, NRect* rect)
//{
//    NStyle style;
//    NRenderNode* rn = N_NULL;
//
//    if (node) {
//        rn = (NRenderNode*) node->render;
//
//        style_init(&style);
//        style.view = page->view;
//        style.dv = view_getRectWithOverflow(page->view);
//        rn->Paint(rn, &style, rect);
//    }
//}

nbool page_isCtrlNode(NPage* page, NNode* node)
{
    NRenderNode* rn = N_NULL;
    NRenderInput* ri = N_NULL;

    nbool bCtrl = N_FALSE;
    if (node) {
        rn = (NRenderNode*) node->render;

        if (rn && RNT_INPUT == rn->type) {

            ri = (NRenderInput*) rn;
            if (ri) {
                switch (ri->type) {
                case NEINPUT_SUBMIT:
                case NEINPUT_RESET:
                case NEINPUT_BUTTON:
                case NEINPUT_IMAGE:
                case NEINPUT_RADIO:
                case NEINPUT_CHECKBOX:
                    bCtrl = N_TRUE;
                    break;
                }
            }
        }
        else if (rn && RNT_SELECT == rn->type) {
            bCtrl = N_TRUE;
        }
    }

    return bCtrl;
}

NPage* page_getMainPage(NPage* page)
{
    NPage* p = page;
    while (p->parent)
        p = p->parent;
    return p;
}

nid page_getStopId(NPage* page)
{
    NHistory* history = (NHistory*)page->history;
    return (history->preId) ? history->preId : page->id;
}

void page_layout(NPage* page, nbool force)
{
    view_layout(page->view, page->view->root, force);
    if (page->subPages) {
        NPage* p = (NPage*)sll_first(page->subPages);
        while (p) {
            if (p->attach)
                view_layout(p->view, p->view->root, force);
            p = (NPage*)sll_next(page->subPages);
        }
    }
}
