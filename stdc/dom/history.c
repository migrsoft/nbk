/*
 * history.c
 *
 *  Created on: 2011-5-8
 *      Author: migr
 */

#include "history.h"
#include "document.h"
#include "view.h"
#include "xml_tags.h"
#include "xml_atts.h"
#include "char_ref.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_callback.h"
#include "../render/imagePlayer.h"
#include "../loader/loader.h"
#include "../loader/url.h"
#include "../tools/str.h"
#include "../tools/memMgr.h"
#include "../css/css_prop.h"
#include "../css/css_value.h"
#include "../css/color.h"

static nid l_page_id = 0;

static int l_globe_refer = 0;

static void globe_init(void)
{
    if (l_globe_refer == 0) {
#ifdef NBK_USE_MEMMGR
		memMgr_init();
#endif
        xml_initTags();
        xml_initAtts();
        xml_initEntityTable();
        css_initProp();
        css_initVals();
        css_initColor();
    }
    
    l_globe_refer++;
}

static void globe_destory(void)
{
    l_globe_refer--;
    
    if (l_globe_refer == 0) {
        xml_delTags();
        xml_delAtts();
        xml_delEntityTable();
        css_delProp();
        css_delVals();
        css_delColor();
#ifdef NBK_USE_MEMMGR
		memMgr_quit();
#endif
    }
}

NHistory* history_create(void* pfd)
{
    NHistory* h = (NHistory*)NBK_malloc0(sizeof(NHistory));

    globe_init();
        
    h->list[0].zoom = 1.0f;
    
    h->cur = -1;
    h->page = page_create(N_NULL, pfd);
    
    return h;
}

static void clear_item(NHistoryItem* item, void* pfd)
{
    NBK_resourceClean(pfd, item->id);

    if (item->url) {
        NBK_free(item->url);
        item->url = N_NULL;
    }

    if (item->formData)
        formData_delete(&item->formData);
}

void history_delete(NHistory** history)
{
    NHistory* h = *history;
    int i;

    if (h->cur == h->max) {
        // 记录表单数据
        if (h->list[h->cur].formData == N_NULL)
            h->list[h->cur].formData = formData_create();
        formData_save(h->list[h->cur].formData, h->page);
    }

    for (i=0; i <= h->max; i++)
        clear_item(&h->list[i], h->page->platform);
    
    if (h->page)
        page_delete(&h->page);
    
    NBK_free(h);
    *history = N_NULL;
    
    globe_destory();
}

NPage* history_curr(NHistory* history)
{
    if (history->cur == -1) {
        history->cur++;
        history->max = history->cur;
        history->list[history->cur].id = ++l_page_id;
        page_setId(history->page, history->list[history->cur].id);
        history->page->history = history;
    }
    
    return history->page;
}

// 记录页面信息
static void remember_page_info(NHistory* history)
{
    NRect va;
    NBK_helper_getViewableRect(history->page->platform, &va);

    // 记录文档编码
    history->list[history->cur].charset = history->page->encoding;

    // 记录最后阅读位置
    history->list[history->cur].hasReadInfo = N_TRUE;
    history->list[history->cur].lastPos.x = va.l; // view coordinate
    history->list[history->cur].lastPos.y = va.t;
    history->list[history->cur].lastZoom = history->list[history->cur].zoom;

    // 记录表单数据
    if (history->list[history->cur].formData == N_NULL)
        history->list[history->cur].formData = formData_create();
    formData_save(history->list[history->cur].formData, history->page);
}

void history_add(NHistory* history)
{
    int i;
    NPoint pos = {0, 0};
    
    if (history->preId == 0)
        history->preId = ++l_page_id;

    if (history->cur >= 0)
        remember_page_info(history);
    
    // 删除空白页
    if (history->cur >= 0 && history->page->doc->words == 0) {
        clear_item(&history->list[history->cur], history->page->platform);
        
        for (i = history->cur; i < history->max; i++)
            history->list[i] = history->list[i + 1];
        history->cur--;
        history->max--;
    }
    
    if (   (history->max < NBK_HISTORY_MAX - 1)
        || (history->max == NBK_HISTORY_MAX - 1 && history->cur < history->max) ) {
        
        // delete records after current
        for (i = history->cur + 1; i <= history->max; i++) {
            clear_item(&history->list[i], history->page->platform);
            history->list[i].id = 0;
            history->list[i].hasReadInfo = N_FALSE;
        }
        history->cur++;
    }
    else {
        // shift records left
        clear_item(&history->list[0], history->page->platform);
        for (i=0; i < history->cur; i++)
            history->list[i] = history->list[i + 1];
    }
    
    // new item
    NBK_memset(&history->list[history->cur], 0, sizeof(NHistoryItem));
    history->list[history->cur].id = history->preId;
    history->list[history->cur].zoom = 1.0f;

    history->list[history->cur].hasReadInfo = N_FALSE;
    
    page_setId(history->page, history->list[history->cur].id);
    history->max = history->cur;
    history->preId = 0;
}

nbool history_prev(NHistory* history)
{
    if (history->cur < 1) {
        return N_FALSE;
    }
    else {
        loader_stopAll(history->page->platform, page_getStopId(history->page));
        remember_page_info(history);
        history->cur--;
        page_setId(history->page, history->list[history->cur].id);
        page_loadFromCache(history->page);
        return N_TRUE;
    }
}

nbool history_next(NHistory* history)
{
    if (history->cur >= history->max) {
        return N_FALSE;
    }
    else {
        loader_stopAll(history->page->platform, page_getStopId(history->page));
        remember_page_info(history);
        history->cur++;
        page_setId(history->page, history->list[history->cur].id);
        page_loadFromCache(history->page);
        return N_TRUE;
    }
}

nbool history_goto(NHistory* history, int16 idx)
{
    int i = idx - 1;
    if (i >= 0 && i <= history->max) {
        loader_stopAll(history->page->platform, page_getStopId(history->page));
        remember_page_info(history);
        history->cur = i;
        page_setId(history->page, history->list[history->cur].id);
        page_loadFromCache(history->page);
        return N_TRUE;
    }
    else
        return N_FALSE;
}

nid history_getPreId(NHistory* history)
{
    history->preId = ++l_page_id;
    return history->preId;
}

void history_getRange(NHistory* history, int16* cur, int16* used)
{
    int16 c, u;
    
    c = history->cur;
    u = history->max;
    
    if (history->page->doc->root) {
        c++;
        u++;
    }
    
    *cur = c;
    *used = u;
}

void history_setZoom(NHistory* history, float zoom)
{
    if (history->cur != -1 /*&& !history->mbMode*/)
        history->list[history->cur].zoom = zoom;
}

float history_getZoom(NHistory* history)
{
	// fixme: 当产生历史操作后，新数据抵达之前，该zoom不与当前页对应
	if (history->cur != -1)
		return history->list[history->cur].zoom;
	else
		return 1.0f;
}

float history_getZoomForPrevPage(NHistory* history)
{
    N_ASSERT(history->cur > 0);
    return history->list[history->cur - 1].zoom;
}

void history_setSettings(NHistory* history, NSettings* settings)
{
    history->page->settings = settings;
}

void history_enablePic(NHistory* history, nbool enable)
{
    if (history->page)
        page_enablePic(history->page, enable);
}

nbool history_isLastPage(NHistory* history)
{
    return (history->cur == history->max) ? N_TRUE : N_FALSE;
}

void history_setUrl(NHistory* history, const char* url)
{
    if (   /*!history->mbMode
        &&*/ url
        && history->cur >= 0
        && history->cur <= history->max )
    {
        if (history->list[history->cur].url)
            NBK_free(history->list[history->cur].url);
        history->list[history->cur].url = (char*)NBK_malloc(nbk_strlen(url) + 2);
        nbk_strcpy(history->list[history->cur].url, url);
    }
}

char* history_getUrl(NHistory* history, nid pageId)
{
	int16 i;
	for (i=0; i <= history->max; i++) {
		if (history->list[i].id == pageId)
			return history->list[i].url;
	}
	return N_NULL;
}

void history_setPaintParams(NHistory* history, int maxTime, nbool drawPic, nbool drawBgimg)
{
    history->page->view->paintMaxTime = maxTime;
    history->page->view->drawPic = drawPic;
    history->page->view->drawBgimg = drawBgimg;
}

NPoint history_getReadInfo(NHistory* history, float* zoom)
{
    if (history->list[history->cur].hasReadInfo) {
        *zoom = history->list[history->cur].lastZoom;
        return history->list[history->cur].lastPos;
    }
    else {
        NPoint pos = {0, 0};
        *zoom = history->list[history->cur].zoom;
        return pos;
    }
}

void history_setRequestMode(NHistory* history, nid mode)
{
    N_ASSERT(history->cur >= 0);
    history->list[history->cur].reqMode = mode;
}

nbool history_getLastRequestMode(NHistory* history, nid* mode)
{
    nbool ret = N_FALSE;
    if (history->cur > 0) {
        *mode = history->list[history->cur - 1].reqMode;
        ret = N_TRUE;
    }
    return ret;
}

void history_restoreFormData(NHistory* history, NPage* page)
{
    NFormData* data = history->list[history->cur].formData;

    if (data)
        formData_restore(data, page);
    else
        formData_restoreLoginData(page);
}

nid history_getCharset(NHistory* history)
{
    nid charset = NEENC_UNKNOWN;
    if (history->cur >= 0 && history->cur <= history->max)
        charset = history->list[history->cur].charset;
    return charset;
}
