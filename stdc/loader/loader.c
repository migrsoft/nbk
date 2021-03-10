/*
 * loader.c
 *
 *  Created on: 2011-2-3
 *      Author: wuyulun
 */

#include "loader.h"
#include "../dom/page.h"
#include "../dom/document.h"
#include "../render/imagePlayer.h"

NRequest* loader_createRequest(void)
{
    NRequest* req = (NRequest*)NBK_malloc0(sizeof(NRequest));
    N_ASSERT(req);
	smartptr_init(req, (SP_FREE_FUNC)loader_deleteRequest);
    return req;
}

void loader_deleteRequest(NRequest** req)
{
    NRequest* r = *req;
    
    if (r->ownUrl && r->url)
        NBK_free(r->url);
    if (r->ownUser && r->user)
        NBK_free(r->user);
    if (r->ownReferer && r->referer)
        NBK_free(r->referer);
    
    if (r->body)
        NBK_free(r->body);
    if (r->upcmd)
        upcmd_delete(&r->upcmd);
    
    if (r->files)
        NBK_free(r->files);
    
    NBK_free(r);
    *req = N_NULL;
}

void loader_setRequest(NRequest* req, uint8 type, char* url, void* user)
{
    N_ASSERT(req);
    req->type = type;
    req->url = url;
    req->ownUrl = N_FALSE;
    req->user = user;
    req->ownUser = N_FALSE;
}

void loader_setRequestUrl(NRequest* req, char* url, nbool own)
{
    N_ASSERT(req);
    
    if (req->url && req->ownUrl)
        NBK_free(req->url);
    
    req->url = url;
    req->ownUrl = own;
}

void loader_setRequestUser(NRequest* req, void* user)
{
    N_ASSERT(req);

    if (req->user && req->ownUser)
        NBK_free(req->user);

    req->user = user;
    req->ownUser = 1;
}

void loader_setRequestReferer(NRequest* req, char* referer, nbool own)
{
    N_ASSERT(req);

    if (req->referer && req->ownReferer)
        NBK_free(req->referer);

    req->referer = referer;
    req->ownReferer = own;
}

nbool loader_load(NRequest* request)
{
    return NBK_resourceLoad(request->platform, request);
}

void loader_stopAll(void* pfd, nid pageId)
{
    NBK_resourceStopAll(pfd, pageId);
}

void loader_onReceiveHeader(NRequest* req, NRespHeader* header)
{
    if (req->responseCallback)
        req->responseCallback(NEREST_HEADER, req, header, 0, 0);
}

void loader_onReceiveData(NRequest* req, char* data, int length, int comprLen)
{
    if (req->responseCallback)
        req->responseCallback(NEREST_DATA, req, (void*)data, length, comprLen);
}

void loader_onComplete(NRequest* req)
{
    if (req->responseCallback)
        req->responseCallback(NEREST_COMPLETE, req, N_NULL, 0, 0);
}

void loader_onCancel(NRequest* req)
{
    if (req->responseCallback)
        req->responseCallback(NEREST_CANCEL, req, N_NULL, 0, 0);
}

void loader_onError(NRequest* req, int error)
{
    if (req->responseCallback)
        req->responseCallback(NEREST_ERROR, req, N_NULL, error, 0);
}
