#include "cssMgr.h"
#include "../dom/document.h"
#include "../dom/page.h"
#include "../dom/history.h"
#include "../tools/str.h"

enum NExtCssState {
    NEXTCSS_NONE,
    NEXTCSS_PENDING,
    NEXTCSS_LOADING,
    NEXTCSS_READY,
    NEXTCSS_ERROR,
    NEXTCSS_CANCEL
};

// -----------------------------------------------------------------------------
// 外部样式表装载管理器
// -----------------------------------------------------------------------------

NExtCssMgr* extCssMgr_create(void* doc)
{
    NExtCssMgr* mgr = (NExtCssMgr*)NBK_malloc0(sizeof(NExtCssMgr));
    N_ASSERT(mgr);

	smartptr_init(mgr, (SP_FREE_FUNC)extCssMgr_delete);
    mgr->cur = -1;
    mgr->doc = doc;
    return mgr;
}

void extCssMgr_delete(NExtCssMgr** mgr)
{
    NExtCssMgr* m = *mgr;
    int i;
    
    if (m->cached)
        strBuf_delete(&m->cached);
    
    for (i=0; i < MAX_EXT_CSS_NUM; i++) {
        if (m->d[i].url)
            NBK_free(m->d[i].url);
        if (m->d[i].dat)
            strBuf_delete(&m->d[i].dat);
    }
    
    NBK_free(m);
    *mgr = N_NULL;
}

// 测试：阻止样式功能
static nbool extCssMgr_ignore(NExtCssMgr* mgr)
{
	NDocument* doc = (NDocument*)mgr->doc;
	if (doc->type == NEDOC_HTML)
		return N_TRUE;
	else
		return N_FALSE;
}

// 添加外部样式表URL
nbool extCssMgr_addExtCss(NExtCssMgr* mgr, char* url)
{
    int i;

	if (extCssMgr_ignore(mgr))
		return N_FALSE;
    
    for (i=0; i < MAX_EXT_CSS_NUM; i++)
        if (mgr->d[i].url && nbk_strcmp(mgr->d[i].url, url) == 0)
            return N_FALSE;
    
    for (i=0; i < MAX_EXT_CSS_NUM; i++)
        if (mgr->d[i].url == N_NULL) {
            mgr->d[i].url = url;
            mgr->d[i].state = NEXTCSS_PENDING;
            mgr->ready = N_FALSE;
            return N_TRUE;
        }
    
    return N_FALSE;
}

// 添加内部样式表文本
void extCssMgr_addInnCss(NExtCssMgr* mgr, const char* style, int length)
{
	if (extCssMgr_ignore(mgr))
		return;

    if (mgr->cached == N_NULL)
        mgr->cached = strBuf_create();
    
    strBuf_appendStr(mgr->cached, style, length);
}

nbool extCssMgr_get(NExtCssMgr* mgr, char** style, int* len, nbool begin)
{
    mgr->use = (begin) ? 0 : mgr->use + 1;
    
    while (mgr->use < MAX_EXT_CSS_NUM) {
        if (mgr->d[mgr->use].state == NEXTCSS_READY) {
            if (strBuf_getStr(mgr->d[mgr->use].dat, style, len, N_TRUE))
                return N_TRUE;
        }
        mgr->use++;
    }
    
    if (mgr->use == MAX_EXT_CSS_NUM && mgr->cached) {
        if (strBuf_getStr(mgr->cached, style, len, N_TRUE))
            return N_TRUE;
    }
    
    return N_FALSE;
}

static void extCssMgr_onResponse(NEResponseType type, void* user, void* data, int v1, int v2)
{
    NRequest* req = (NRequest*)user;
    NExtCssMgr* mgr = (NExtCssMgr*)req->user;
    
    if (mgr->cur < 0 || mgr->cur >= MAX_EXT_CSS_NUM)
        return;
    
    switch(type) {
    case NEREST_HEADER:
    {
        NRespHeader* hdr = (NRespHeader*)data;
        if (hdr->mime == NEMIME_DOC_CSS) {
            if (mgr->d[mgr->cur].dat)
                strBuf_delete(&mgr->d[mgr->cur].dat);
            mgr->d[mgr->cur].dat = strBuf_create();
        }
        else
            mgr->d[mgr->cur].state = NEXTCSS_ERROR;
        break;
    }
        
    case NEREST_DATA:
    {
        if (mgr->d[mgr->cur].state == NEXTCSS_LOADING && mgr->d[mgr->cur].dat)
            strBuf_appendStr(mgr->d[mgr->cur].dat, (char*)data, v1);
        break;
    }
        
    case NEREST_COMPLETE:
    {
        if (mgr->d[mgr->cur].state == NEXTCSS_LOADING)
            mgr->d[mgr->cur].state = NEXTCSS_READY;
        extCssMgr_schedule(mgr);
        break;
    }
        
    case NEREST_ERROR:
    case NEREST_CANCEL:
    {
        mgr->d[mgr->cur].state = NEXTCSS_ERROR;
        extCssMgr_schedule(mgr);
        break;
    }
    default:
        break;
    } // switch
}

static void extCssMgr_do_work(void* user)
{
    NExtCssMgr* mgr = (NExtCssMgr*)user;
    NDocument* doc = (NDocument*)mgr->doc;
    NPage* page = (NPage*)doc->page;
    int i;

	smartptr_release(mgr);
    
    // 检测是否存在下载项（仅允许一个下载项）
    for (i=0; i < MAX_EXT_CSS_NUM; i++)
        if (mgr->d[i].state == NEXTCSS_LOADING)
            return;

    // 选取一个未下载项
    for (i=0; i < MAX_EXT_CSS_NUM; i++) {
        if (mgr->d[i].url && mgr->d[i].state == NEXTCSS_PENDING) {
            NRequest* req = loader_createRequest();
            req->pageId = page->id;
            loader_setRequest(req, NEREQT_CSS, mgr->d[i].url, mgr);
            loader_setRequestReferer(req, doc_getUrl(doc), N_FALSE);
            req->responseCallback = extCssMgr_onResponse;
            req->platform = page->platform;
            if (!history_isLastPage((NHistory*)page->history))
                req->method = NEREM_HISTORY;
            
            if (loader_load(req)) {
                mgr->d[i].state = NEXTCSS_LOADING;
                mgr->cur = i;
                return;
            }
            else {
                smartptr_release(req);
                mgr->d[i].state = NEXTCSS_ERROR;
            }
        }
    }
        
    if (mgr->cached) // 拼接内部样式文本
        strBuf_joinAllStr(mgr->cached);
    for (i=0; i < MAX_EXT_CSS_NUM; i++) {
        if (mgr->d[i].state == NEXTCSS_READY) // 拼接外部样式文本
            strBuf_joinAllStr(mgr->d[i].dat);
    }
    mgr->ready = N_TRUE;

    doc_scheduleLayout(doc);
}

void extCssMgr_schedule(NExtCssMgr* mgr)
{
	NPage* p = (NPage*)((NDocument*)mgr->doc)->page;
	smartptr_get(mgr);
	NBK_callLater(p->platform, p->id, extCssMgr_do_work, mgr);
}

nbool extCssMgr_ready(NExtCssMgr* mgr)
{
    return mgr->ready;
}

void extCssMgr_stop(NExtCssMgr* mgr)
{
    int i;
    for (i=0; i < MAX_EXT_CSS_NUM; i++) {
        if (mgr->d[i].url && mgr->d[i].state == NEXTCSS_PENDING)
            mgr->d[i].state = NEXTCSS_CANCEL;
    }
}
