/*
 * imagePlayer.c
 *
 *  Created on: 2011-2-21
 *      Author: wuyulun
 */

#include "../inc/nbk_limit.h"
#include "../inc/nbk_callback.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_gdi.h"
#include "../css/color.h"
#include "../css/css_val.h"
#include "../css/css_helper.h"
#include "../tools/str.h"
#include "../tools/dump.h"
#include "../loader/url.h"
#include "../loader/upCmd.h"
//#include "../loader/param.h"
#include "../dom/history.h"
#include "../dom/page.h"
#include "../dom/document.h"
#include "../dom/view.h"
#include "../dom/node.h"
#include "imagePlayer.h"

#define DEBUG_SCHEDULE      0

static NImage* image_create(char* src, int8 type)
{
    NImage* im = (NImage*)NBK_malloc0(sizeof(NImage));
    N_ASSERT(im);
    im->src = src;
    im->state = NEIS_PENDING;
    im->type = type;
    im->mime = NEMIME_UNKNOWN;
    return im;
}

static void image_delete(NImage** image)
{
    NImage* im = *image;
    NBK_free(im);
    *image = N_NULL;
}

static nbool image_compareSrc(NImage* image, const char* src)
{
    return (nbk_strcmp(image->src, src) ? N_FALSE : N_TRUE);
}

static nbool image_getSize(NImage* image, NSize* size, nbool* fail)
{
    switch (image->state) {
    case NEIS_DECODE_FINISH:
        *size = image->size;
        *fail = N_FALSE;
        return N_TRUE;
        
    case NEIS_LOAD_FAILED:
    case NEIS_DECODE_FAILED:
        *fail = N_TRUE;
        return N_FALSE;
        
    default:
        *fail = N_FALSE;
        return N_FALSE;
    }
}

// NImagePlayer

static NPage* imagePlayer_getPage(NImagePlayer* player)
{
	return (NPage*)((NView*)player->view)->page;
}

int imagePlayer_total(NImagePlayer* player)
{
    return player->list->use;
}

static int mp_processed(NImagePlayer* player)
{
    int num = 0;
    int16 i;
    NImage* im;
    
    for (i=0; i < player->list->use; i++) {
        im = (NImage*)player->list->data[i];
        if (   im->type == NEIT_FLASH_IMAGE
            || im->state > NEIS_LOAD_NO_CACHE )
            num++;
    }
    return num;
}

static void mp_download_finish(NImagePlayer* player)
{
    if (!player->dlFinish) {
        NView* view = (NView*)player->view;
        NPage* page = (NPage*)view->page;
        NBK_Event_GotImage evt;
        player->dlFinish = 1;
        evt.curr = evt.total = imagePlayer_total(player);
        nbk_cb_call(NBK_EVENT_DOWNLOAD_IMAGE, &page->cbEventNotify, &evt);
    }
}

void imagePlayer_progress(NImagePlayer* player)
{
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    NBK_Event_GotImage evt;
    
    if (player->stop || player->dlFinish)
        return;

    evt.curr = mp_processed(player);
    evt.total = imagePlayer_total(player);
    
#if DEBUG_SCHEDULE
    {
        char buf[64];
        sprintf(buf, "imageP: progress %d / %d", evt.curr, evt.total);
        dump_char(page, buf, -1);
        dump_return(page);
    }
#endif
    
    if (evt.curr >= evt.total)
        mp_download_finish(player);
    else {
        if (player->loadByInvoke)
            evt.curr = evt.total = 0;

        nbk_cb_call(NBK_EVENT_DOWNLOAD_IMAGE, &page->cbEventNotify, &evt);
    }
}

static nbool mp_in_working(NImagePlayer* player, int8 state)
{
    int i;
    NImage* im;
    
    for (i=0; i < player->list->use; i++) {
        im = (NImage*)player->list->data[i];
        if (im->state == state)
            return N_TRUE;
    }
    
    return N_FALSE;
}

static NImage* mp_get_image_to_decode(NImagePlayer* player)
{
	int i;
	NImage* im = N_NULL;
	NPage* page = (NPage*)((NView*)player->view)->page;

	for (i=0; i < player->list->use; i++, im = N_NULL) {
		im = (NImage*)player->list->data[i];
		if (im->state == NEIS_LOAD_FINISH)
			break;
	}

	return im;
}

static void mp_notify_pics_changed(NImagePlayer* player)
{
    int i;
    NImage* im;
    NSList* lst = sll_create();

    for (i=0; i < player->list->use; i++) {
        im = (NImage*)player->list->data[i];
        if (im->state == NEIS_DECODE_FINISH && !im->notify) {
            im->notify = 1;
            sll_append(lst, (void*)(i + 1));
        }
    }

    view_picsChanged((NView*)player->view, lst);

    sll_delete(&lst);
}

static void imagePlayer_schedule_load(void* user)
{
    NImagePlayer* player = (NImagePlayer*)user;
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    NImage* image;
    char* url = N_NULL;
    NRequest* req;
    NImageRequest* imgReq;
    int16 i;
    
	smartptr_release(player);
    
    if (player->stop)
        return;

	if (mp_in_working(player, NEIS_LOADING))
		return;

    // download one by one
    
    for (i=0; i < player->list->use; i++) {
//        nid state = (player->reqRest) ? NEIS_HAS_CACHE : NEIS_PENDING;
        image = (NImage*)player->list->data[i];
        if (image->type == NEIT_FLASH_IMAGE)
            continue;
        if (   image->state == NEIS_PENDING
            /*|| image->state == NEIS_HAS_CACHE*/
            /*|| image->state == NEIS_MP_LOADING*/)
            break;
    }
    
    if (i >= player->list->use) {
        // all images load finish.
        
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: NO IMAGE NEED LOAD!", -1);
        dump_return(page);
#endif

        mp_download_finish(player);
        imagePlayer_aniResume(player);
        return;
    }
    
#if DEBUG_SCHEDULE
    dump_char(page, "imageP: schedule load...", -1);
    dump_return(page);
#endif
    
    image = (NImage*)player->list->data[i];
    image->state = NEIS_LOADING;
    
    url = url_parse(doc_getUrl(page->doc), image->src);
    
    imgReq = (NImageRequest*)NBK_malloc(sizeof(NImageRequest));
    imgReq->image = image;
    imgReq->player = player;
    
    req = loader_createRequest();
    req->referer = doc_getUrl(page->doc);
    req->pageId = page->id;
    if (player->cache) req->method = NEREM_HISTORY;
    
    loader_setRequest(req, NEREQT_IMAGE, N_NULL, N_NULL);
    loader_setRequestUrl(req, url, N_TRUE);
    loader_setRequestUser(req, imgReq);
    req->responseCallback = imagePlayer_onLoadResponse;
    req->platform = page->platform;
    
#if DEBUG_SCHEDULE
    dump_char(page, "imageP: load", -1);
    dump_int(page, i);
    dump_int(page, (int)req->method);
    dump_char(page, url, -1);
    dump_return(page);
#endif
    
    if (!loader_load(req)) {
        image->state = NEIS_LOAD_FAILED;
        smartptr_release(req);
        imagePlayer_start(player);
    }
}

//static void imagePlayer_schedule_load_cache(void* user)
//{
//    NImagePlayer* player = (NImagePlayer*)user;
//    NView* view = (NView*)player->view;
//    NPage* page = (NPage*)view->page;
//    NImage* image = N_NULL;
//    NRequest* req;
//    NImageRequest* imgReq;
//    int16 i;
//    char* url;
//    
//	smartptr_release(player);
//    
//    if (player->stop)
//        return;
//    
//	if (mp_in_working(player, NEIS_LOADING))
//		return;
//
//    for (i=0; i < player->list->use; i++, image = N_NULL) {
//        image = (NImage*)player->list->data[i];
//        if (image->type != NEIT_FLASH_IMAGE && image->state == NEIS_HAS_CACHE)
//            break;
//    }
//    
//    if (image == N_NULL) {
//#if DEBUG_SCHEDULE
//        dump_char(page, "imageP: *** LOAD CACHE FINISH! request rest from net...", -1);
//        dump_return(page);
//#endif
//		player->loadCache = N_FALSE;
//        imagePlayer_start(player);
//        return;
//    }
//    
//    image->state = NEIS_LOADING;
//    url = url_parse(doc_getUrl(page->doc), image->src);
//    
//    imgReq = (NImageRequest*)NBK_malloc(sizeof(NImageRequest));
//    imgReq->image = image;
//    imgReq->player = player;
//    
//    req = loader_createRequest();
//	smartptr_init(req, (SP_FREE_FUNC)loader_deleteRequest);
//    req->referer = doc_getUrl(page->doc);
//    req->pageId = page->id;
//    if (player->cache) req->method = NEREM_HISTORY;
//
//    loader_setRequest(req, NEREQT_IMAGE, N_NULL, N_NULL);
//    loader_setRequestUrl(req, url, N_TRUE);
//    loader_setRequestUser(req, imgReq);
//    req->responseCallback = imagePlayer_onLoadResponse;
//    req->platform = page->platform;
//    
//#if DEBUG_SCHEDULE
//    dump_char(page, "imageP: load from cache", -1);
//    dump_int(page, i);
//    dump_int(page, (int)req->method);
//    dump_char(page, url, -1);
//    dump_return(page);
//#endif
//    
//    if (!loader_load(req)) {
//        image->state = NEIS_LOAD_FAILED;
//        smartptr_release(req);
//        imagePlayer_start(player);
//    }
//}

static void imagePlayer_schedule_animate(void* user)
{
    NImagePlayer* player = (NImagePlayer*)user;
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    NImage* im;
    int16 i;
    nbool ant = N_FALSE;
    
    tim_stop(player->aniTimer);
    
    if (player->stop)
        return;
    
    for (i=0; i < player->list->use; i++) {
        im = (NImage*)player->list->data[i];
        if (im->state == NEIS_DECODE_FINISH && im->total > 1) {
            im->curr = (im->curr + 1 == im->total) ? 0 : im->curr + 1;
            ant = N_TRUE;
        }
    }
    
    if (ant) {
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: animating...", -1);
        dump_return(page);
#endif
        view_picUpdate(view, -1);
        tim_start(player->aniTimer, 1000, 5000);
    }
}

static void imagePlayer_schedule_decode(void* user)
{
    NImagePlayer* player = (NImagePlayer*)user;
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    NImage* im;

	smartptr_release(player);

    im = mp_get_image_to_decode(player);

    if (im) {
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: decoding...", -1);
        dump_int(page, im->id);
        dump_return(page);
#endif
        im->state = NEIS_DECODING;
        // 仅当只有一张图片时，强制解码
        im->mustDecode = (player->list->use == 1) ? 1 : 0;
        NBK_imageDecode(page->platform, player, im);
    }
}

// 检测可用内存。当可用内存少于指定阀值时，停止图片相关操作
static nbool imagePlayer_checkMemory(NImagePlayer* player)
{
    if (NBK_currentFreeMemory() < MINI_MEMORY_FOR_PIC_DECODE) {
        NView* view = (NView*)player->view;
        NPage* page = (NPage*)view->page;
        
        mp_download_finish(player);        
        dump_time(page);
        dump_char(page, "ImageP [ memory insufficient ]", -1);
        dump_return(page);
        dump_flush(page);
        
        player->stop = 1;
        return N_FALSE;
    }
    else
        return N_TRUE;
}

// 启动图片解码
void imagePlayer_startDecode(NImagePlayer* player)
{
    NImage* im;
    NPage* page = imagePlayer_getPage(player);
    
    if (!imagePlayer_checkMemory(player))
        return;
    
    if (mp_in_working(player, NEIS_DECODING))
        return;

    im = mp_get_image_to_decode(player);
    
    if (im) {
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: start decode...", -1);
        dump_return(page);
#endif
		smartptr_get(player);
		NBK_callLater(page->platform, page_getId(page), imagePlayer_schedule_decode, player);
    }
    else if (player->numDecoded) {
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: NO IMAGE NEED DECODE!", -1);
        dump_int(page, player->numDecoded);
        dump_return(page);
#endif
        mp_notify_pics_changed(player);
        player->numDecoded = 0;
    }
}

NImagePlayer* imagePlayer_create(void* view)
{
	NPage* p = (NPage*)((NView*)view)->page;
    NImagePlayer* player = (NImagePlayer*)NBK_malloc0(sizeof(NImagePlayer));
    N_ASSERT(player);

	smartptr_init(player, (SP_FREE_FUNC)imagePlayer_delete);
	player->view = view;
    player->list = ptrArray_create();
	player->aniTimer = tim_create(imagePlayer_schedule_animate, player, p->platform);
    
    return player;
}

void imagePlayer_delete(NImagePlayer** player)
{
    NImagePlayer* pl = *player;
    
    imagePlayer_reset(pl);
    
    tim_delete(&pl->aniTimer);
    
    ptrArray_delete(&pl->list);
    
    NBK_free(pl);
    *player = N_NULL;
}

void imagePlayer_reset(NImagePlayer* player)
{
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    int16 i;
    NImage* im;
    
    if (page->platform == N_NULL)
        return;
    
    tim_stop(player->aniTimer);
    
    NBK_imageStopAll(page->platform);
    for (i=0; i < player->list->use; i++) {
        im = (NImage*)player->list->data[i];
        NBK_imageDelete(page->platform, player, im);
        image_delete(&im);
    }
    ptrArray_reset(player->list);
    
    player->stop = 0;
    player->cache = 0;
    player->dlFinish = 0;
    player->ticks = 0;
    player->time = NBK_currentMilliSeconds() - 1;
    player->interval = 1000;
    player->numDecoded = 0;
	player->loadCache = N_TRUE;

    NBK_imageStopAll(page->platform);
    
#if DEBUG_SCHEDULE
    dump_char(page, "==========", -1);
    dump_return(page);
    dump_flush(page);
#endif
}

int16 imagePlayer_getId(NImagePlayer* player, const char* src, int8 type)
{
    int16 i;
    NImage* image;
    NPage* page = imagePlayer_getPage(player);
	char* u;
    
    if (player->disable)
        return -1; // ignore image request
    
    if (player->loadByInvoke && type != NEIT_IMAGE)
        return -1;
    
    if (nbk_strncmp(src, "about:", 6) == 0)
        return -1;
    
    for (i = 0; i < player->list->use; i++) {
        image = (NImage*)player->list->data[i];
        if (image->state != NEIS_DELETE && image_compareSrc(image, src))
            return i; // already exist
    }
    
    // not found insert new one
    image = image_create((char*)src, type);
    ptrArray_append(player->list, image);
    image->pageId = player->pageId;
    image->id = player->list->use - 1;
    image->state = NEIS_PENDING;
    NBK_imageCreate(page->platform, player, image);
    
    u = url_parse(doc_getUrl(page->doc), src);

    //if (NBK_isImageCached(page->platform, u, (nbool)player->cache))
    //    image->state = NEIS_HAS_CACHE;
    
    NBK_free(u);
    
#if DEBUG_SCHEDULE
    dump_char(page, "ImageP: add", -1);
    dump_int(page, image->id);
    dump_int(page, image->type);
    dump_int(page, image->state);
    dump_char(page, image->src, -1);
    dump_return(page);
#endif
    
    return image->id;
}

nbool imagePlayer_getSize(NImagePlayer* player, int16 id, NSize* size, nbool* fail)
{
    NImage* im;
    
    if (id < 0 || id >= player->list->use)
        return N_FALSE;
    
	im = (NImage*)player->list->data[id];
    
    return image_getSize(im, size, fail);
}

void imagePlayer_draw(NImagePlayer* player, int16 id, const NRect* rect)
{
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    NImage* im;
    
    if (id < 0 || id >= player->list->use)
        return;
    
    im = (NImage*)player->list->data[id];
    im->time = NBK_currentMilliSeconds() - player->time;
    if (   im->type != NEIT_FLASH_IMAGE
        && (   im->state == NEIS_LOAD_FINISH
            || (player->loadByInvoke && im->state < NEIS_DECODE_FINISH) ) ) {
        NColor fill[4] = {{228, 197, 184, 255},
                          {241, 242, 242, 255},
                          {229, 222, 200, 255},
                          {225, 240, 219, 255}};
        NBK_gdi_setBrushColor(page->platform, &fill[id % 4]);
        NBK_gdi_fillRect(page->platform, rect);
        if (!player->loadByInvoke)
            imagePlayer_startDecode(player);
    }
    else
        NBK_imageDraw(page->platform, player, im, rect);
}

void imagePlayer_drawBg(NImagePlayer* player, const NImageDrawInfo di)
{
    NPage* page = (NPage*)((NView*)player->view)->page;
    NImage* im;
    coord x, y, w, h, iw, ih;
    NRect r, cl;
    int rx = 0, ry = 0;

    if (di.id < 0 || di.id >= player->list->use)
        return;
    
	im = (NImage*)player->list->data[di.id];
    if (im->state == NEIS_LOAD_FINISH) {
        imagePlayer_startDecode(player);
        return;
    }
    if (im->state != NEIS_DECODE_FINISH)
        return;
    
    w = rect_getWidth(di.vr);
    h = rect_getHeight(di.vr);
    
    cl = *(di.pr);
    cl.l = (cl.l < 0) ? 0 : cl.l;
    cl.t = (cl.t < 0) ? 0 : cl.t;
    cl.r = (cl.r > w) ? w : cl.r;
    cl.b = (cl.b > h) ? h : cl.b;
    NBK_gdi_setClippingRect(page->platform, &cl);
    
    x = di.r.l + di.ox;
    y = di.r.t + di.oy;
    
    iw = di.iw;
    ih = di.ih;
    
    // the first area to draw bg
    r.l = x;
    r.t = y;
    r.r = r.l + iw;
    r.b = r.t + ih;
    rect_toView(&r, di.zoom);
    rect_move(&r, r.l - di.vr->l, r.t - di.vr->t);
    
    iw = rect_getWidth(&r);
    ih = rect_getHeight(&r);
    if (iw == 0 || ih == 0)
        return;
    
    w = rect_getWidth(di.pr) + iw;
    if (x < 0) w += iw * 2;
    h = rect_getHeight(di.pr) + ih;
    if (y < 0) h += ih * 2;

    if (di.repeat == CSS_REPEAT_NO) {
        NBK_imageDrawBg(page->platform, player, im, &r, 0, 0);
    }
    else if (di.repeat == CSS_REPEAT_X) {
        rx = w / iw;
        NBK_imageDrawBg(page->platform, player, im, &r, rx, 0);
    }
    else if (di.repeat == CSS_REPEAT_Y) {
        ry = h / ih;
        NBK_imageDrawBg(page->platform, player, im, &r, 0, ry);
    }
    else {
        rx = w / iw;
        ry = h / ih;
        NBK_imageDrawBg(page->platform, player, im, &r, rx, ry);
    }
    
    NBK_gdi_cancelClippingRect(page->platform);
}

void imagePlayer_onLoadResponse(NEResponseType type, void* user, void* data, int v1, int v2)
{
    NRequest* r = (NRequest*)user;
    NImageRequest* req = (NImageRequest*)r->user;
    NImage* image = req->image;
    NImagePlayer* player = req->player;
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    
    switch (type) {
    case NEREST_HEADER:
    {
        NRespHeader* hdr = (NRespHeader*)data;
        image->mime = hdr->mime;
        image->length = hdr->content_length;
        image->received = 0;
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: got header", -1);
        dump_int(page, image->id);
        dump_int(page, image->length);
        if (hdr->user) {
            char bgpos[32];
            sprintf(bgpos, "bgpos(%ld,%ld)", image->cli->new_x, image->cli->new_y);
            dump_char(page, bgpos, -1);
        }
        dump_return(page);
#endif
        break;
    }

    case NEREST_DATA:
    {
        image->received += v1;
        NBK_onImageData(page->platform, image, data, v1);
        break;
    }
        
    case NEREST_COMPLETE:
    {
        if (image->received > 0) {
            image->state = NEIS_LOAD_FINISH;
            NBK_onImageDataEnd(page->platform, image, N_TRUE);
            imagePlayer_startDecode(player);
        }
        else {
            image->state = NEIS_LOAD_FAILED;
            NBK_onImageDataEnd(page->platform, image, N_FALSE);
        }
        imagePlayer_progress(player);
        imagePlayer_start(player);
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: download complete!", -1);
        dump_int(page, image->id);
        dump_int(page, image->received);
        dump_return(page);
#endif
        break;
    }
        
    case NEREST_ERROR:
    case NEREST_CANCEL:
    {
        image->state = (v1 == NELERR_NOCACHE) ? NEIS_LOAD_NO_CACHE : NEIS_LOAD_FAILED;
        NBK_onImageDataEnd(page->platform, image, N_FALSE);
        imagePlayer_progress(player);
        imagePlayer_startDecode(player);
        imagePlayer_start(player);
#if DEBUG_SCHEDULE
        dump_char(page, "imageP: download error, process next one", -1);
        dump_int(page, image->id);
        dump_int(page, image->state);
        dump_return(page);
#endif
        break;
    }
    
    default:
        break;
    }
}

static nbool mp_need_update_view(NImagePlayer* player)
{
    nbool update = N_FALSE;

    if (player->ticks == 0) {
        player->ticks = NBK_currentMilliSeconds();
    }
    else {
        uint32 now = NBK_currentMilliSeconds();
        if (now - player->ticks >= player->interval) {
            player->ticks = now;
            update = N_TRUE;
        }
    }

    return update;
}

void imagePlayer_onDecodeOver(NImagePlayer* player, int16 id, nbool succeed)
{
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page; // always main page
    NImage* im = (NImage*)player->list->data[id];

    im->state = (succeed) ? NEIS_DECODE_FINISH : NEIS_DECODE_FAILED;
    
#if DEBUG_SCHEDULE
    dump_char(page, "imageP: decoded!", -1);
    dump_int(page, id);
    dump_char(page, "ret", -1);
    dump_int(page, succeed);
    dump_return(page);
#endif

    if (succeed) {
		if (im->type == NEIT_CSS_IMAGE) {
            // 背景图更新，仅产生重绘，不产生排版
            view_picUpdate(view, im->id);
        }
        else {
            //view_picChanged(view, im->id); // 每图片更新

            // 直连模式：前景图解码完成后，在一定时间间隔进行批量更新
            player->numDecoded++;
            if (mp_need_update_view(player)) {
                mp_notify_pics_changed(player);
                player->numDecoded = 0;
            }
        }
    }
    
    imagePlayer_startDecode(player);
}

void imagePlayer_start(NImagePlayer* player)
{
	NPage* p = imagePlayer_getPage(player);
    
    if (player->stop || player->loadByInvoke)
        return;

    if (!imagePlayer_checkMemory(player))
        return;
    
	smartptr_get(player);
	//if (player->loadCache)
	//	NBK_callLater(p->platform, page_getId(p), imagePlayer_schedule_load_cache, player);
	//else
		NBK_callLater(p->platform, page_getId(p), imagePlayer_schedule_load, player);
}

void imagePlayer_disable(NImagePlayer* player, nbool disable)
{
    player->disable = disable;
}

void imagePlayer_stopAll(NImagePlayer* player)
{
    NView* view = (NView*)player->view;
    NPage* page = (NPage*)view->page;
    int i;
    
    tim_stop(player->aniTimer);

	player->stop = 1;
    
#if DEBUG_SCHEDULE
    dump_char(page, "ImageP: *** stop all ***", -1);
    dump_return(page);
#endif
    
    for (i=0; i < player->list->use; i++) {
        NImage* im = (NImage*)player->list->data[i];
        
        switch (im->state) {
        case NEIS_LOADING:
        case NEIS_MP_LOADING:
            im->state = NEIS_PENDING;
            break;
        }
    }
}

void imagePlayer_aniPause(NImagePlayer* player)
{
    if (!player->aniTimer->run)
        return;
    
    tim_stop(player->aniTimer);
}

void imagePlayer_aniResume(NImagePlayer* player)
{
    if (player->aniTimer->run)
        return;
    if (!player->dlFinish)
        return;
    
    tim_start(player->aniTimer, 1000, 5000);
}

nbool imagePlayer_isGif(NImagePlayer* player, int16 id)
{
    if (id < 0 || id >= player->list->use)
        return N_FALSE;
    
    return (((NImage*)player->list->data[id])->total > 1) ? N_TRUE : N_FALSE;
}

void imagePlayer_setDraw(NImagePlayer* player, int16 id)
{   
    if (id >= 0 && id < player->list->use) {
		NImage* im = (NImage*)player->list->data[id];
        im->time = NBK_currentMilliSeconds() - player->time;
        if (im->state == NEIS_LOAD_FINISH)
            imagePlayer_startDecode(player);
    }
}

nid imagePlayer_getState(NImagePlayer* player, int16 id)
{
    if (id >=0 && id < player->list->use)
        return ((NImage*)player->list->data[id])->state;
    else
        return NEIS_NONE;
}

void imagePlayer_invalidateId(NImagePlayer* player, int16 id)
{
    if (id >= 0 && id < player->list->use) {
        NImage* im = (NImage*)player->list->data[id];
        im->state = NEIS_DELETE;
    }
}
