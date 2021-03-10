#include "stdafx.h"
#include "LoaderMgr.h"
#include "NbkCore.h"
#include "NConn.h"
#include "NConnFile.h"
#include "NConnHttp.h"
#include "NConnCache.h"
#include "NConnHistory.h"
#include "NbkEvent.h"
#include "../../stdc/tools/str.h"
#include "../../stdc/dom/history.h"

////////////////////////////////////////////////////////////////////////////////
//
// 来自NBK的资源请求
//

nbool NBK_handleError(int error)
{
	return N_FALSE;
}

char* NBK_getStockPage(int type)
{
	if (type == NESPAGE_ERROR_404)
		return "file:///c:/nbk/nbk_404.htm";
	else
		return "file:///c:/nbk/nbk_error.htm";
}

nbool NBK_resourceLoad(void* pfd, NRequest* req)
{
	// 创建历史页面数据目录
	if (req->method == NEREM_NORMAL) {
		char path[64];
		LoaderMgr::GetHistoryPath(req->pageId, path);
		if (!LoaderMgr::IsPathExist(path))
			LoaderMgr::MakeAllDir(path);
	}

	NbkCore* core = (NbkCore*)pfd;
	return core->GetLoader()->Load(req);
}

void NBK_resourceStopAll(void* pfd, nid pageId)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetLoader()->RemoveAllRequests(pageId);
}

void NBK_resourceClean(void* pfd, nid id)
{
	char path[64];
	LoaderMgr::GetHistoryPath(id, path);
	LoaderMgr::RemoveDir(path);
}

////////////////////////////////////////////////////////////////////////////////
//
// LoaderMgr 资源加载管理器
//

static const char* COOKIE_FILE	= "c:\\nbkcache\\cookies.dat";
static const char* CACHE_PATH	= "c:\\nbkcache\\cache\\";

LoaderMgr::LoaderMgr(NbkCore* core)
{
	mCore = core;
	mCounter = 1;
	NBK_memset(&mConns, 0, sizeof(Conn) * MAX_REQUEST);

	mCookieMgr = NULL;
	mCacheMgr = NULL;
}

LoaderMgr::~LoaderMgr()
{
	RemoveAllRequests(0);
}

void LoaderMgr::Init()
{
	InitializeCriticalSection(&mCookieAccessCS);
	mCookieMgr = cookieMgr_create();
	cookieMgr_load(mCookieMgr, COOKIE_FILE);

	InitializeCriticalSection(&mCacheAccessCS);
	mCacheMgr = cacheMgr_create(30);
	cacheMgr_setPath(mCacheMgr, CACHE_PATH);
	cacheMgr_load(mCacheMgr);
}

void LoaderMgr::Quit()
{
	DeleteCriticalSection(&mCookieAccessCS);
	cookieMgr_save(mCookieMgr, COOKIE_FILE);
	cookieMgr_delete(&mCookieMgr);

	DeleteCriticalSection(&mCacheAccessCS);
	cacheMgr_save(mCacheMgr);
	cacheMgr_delete(&mCacheMgr);
}

nbool LoaderMgr::Load(NRequest* request)
{
	int connId = AddRequest(request);
	if (connId == 0)
		return N_FALSE;

	if (request->via == NEREV_TF) {
		const char* tf = "http://gate.baidu.com/tc?bd_page_type=1&src=";
		int len = nbk_strlen(request->url) + nbk_strlen(tf);
		char* u = (char*)NBK_malloc(len + 1);
		sprintf(u, "%s%s", tf, request->url);
		loader_setRequestUrl(request, u, N_TRUE); // 置换请求地址
		request->via = NEREV_STANDARD;
	}

	// 加载缓存
	if (   (request->method == NEREM_NORMAL || request->method == NEREM_HISTORY)
		&& nbk_strncmp(request->url, "http://", 7) == 0 ) {
		int cacheId;
		int mime;
		int encoding;
		int length;
		bool gzip;
		cacheId = FindCache(request->url, &mime, &encoding, &length, &gzip);
		if (cacheId != -1) {
			NConnCache* c = new NConnCache(cacheId, connId, request, this);
			c->SetInfo(mime, encoding, length, gzip);
			c->Submit();
			return N_TRUE;
		}
	}

	if (request->method == NEREM_HISTORY) {
		NConnHistory* c = new NConnHistory(connId, request, this);
		c->Submit();
		return N_TRUE;
	}

	printf("load -> %s\n", request->url);

	if (nbk_strncmp(request->url, "file://", 7) == 0) {
		NConnFile* c = new NConnFile(connId, request, this);
		c->Submit();
		return N_TRUE;
	}
	else if (nbk_strncmp(request->url, "http://", 7) == 0) {
		NConnHttp* c = new NConnHttp(connId, request, this);
		c->Submit();
		return N_TRUE;
	}

	RemoveRequest(request);
	return N_FALSE;
}

int LoaderMgr::AddRequest(NRequest* request)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].id == 0) {
			mConns[i].id = mCounter++; // 生成惟一连接ID
			mConns[i].req = request;
			return mConns[i].id;
		}
	}
	return 0;
}

void LoaderMgr::RemoveRequest(NRequest* request)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].req == request) {
			mConns[i].id = 0;
			mConns[i].req = N_NULL;
			break;
		}
	}
}

NRequest* LoaderMgr::FindRequest(int connId)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].id == connId)
			return mConns[i].req;
	}
	return N_NULL;
}

void LoaderMgr::RemoveAllRequests(nid pageId)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].id && (pageId == 0 || mConns[i].req->pageId == pageId)) {
			smartptr_release(mConns[i].req);
			mConns[i].id = 0;
			mConns[i].req = N_NULL;
		}
	}
}

void LoaderMgr::ReceiveHeader(NConn* conn, int mime, int encoding, int length)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_RECE_HEADER);
	e->SetConnId(conn->GetId());
	e->SetMime(mime);
	e->SetEncoding(encoding);
	e->SetLength(length);
	mCore->PostEvent(e);
}

void LoaderMgr::ReceiveData(NConn* conn, char* data, int length, int comprLen)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_RECE_DATA);
	e->SetConnId(conn->GetId());
	e->SetData(data, length);
	e->SetLength(comprLen);
	mCore->PostEvent(e);
}

void LoaderMgr::ReceiveComplete(NConn* conn)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_RECE_COMPLETE);
	e->SetConnId(conn->GetId());
	mCore->PostEvent(e);
}

void LoaderMgr::ReceiveError(NConn* conn, int error)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_RECE_ERROR);
	e->SetConnId(conn->GetId());
	e->SetError(error);
	mCore->PostEvent(e);
}

void LoaderMgr::HandleEvent(NbkEvent* e)
{
	bool delReq = false;
	NRequest* req = FindRequest(e->GetConnId());
	if (req == NULL)
		return;

	switch (e->GetId()) {
	case NbkEvent::EVENT_RECE_HEADER:
		NRespHeader hdr;
		NBK_memset(&hdr, 0, sizeof(NRespHeader));
		hdr.mime = e->GetMime();
		hdr.encoding = e->GetEncoding();
		hdr.content_length = e->GetLength();
		loader_onReceiveHeader(req, &hdr);
		break;

	case NbkEvent::EVENT_RECE_DATA:
		loader_onReceiveData(req, e->GetData(), e->GetDataLen(), e->GetLength());
		break;

	case NbkEvent::EVENT_RECE_COMPLETE:
		loader_onComplete(req);
		delReq = true;
		break;

	case NbkEvent::EVENT_RECE_ERROR:
		loader_onError(req, e->GetError());
		delReq = true;
		break;
	}

	if (delReq) {
		RemoveRequest(req);
		smartptr_release(req);
	}
}

char* LoaderMgr::GetCookie(const char* url)
{
	EnterCriticalSection(&mCookieAccessCS);
	char* c = cookieMgr_getCookie(mCookieMgr, url);
	LeaveCriticalSection(&mCookieAccessCS);
	return c;
}

void LoaderMgr::SetCookie(const char* host, char* cookie)
{
	EnterCriticalSection(&mCookieAccessCS);
	cookieMgr_setCookie(mCookieMgr, host, cookie);
	LeaveCriticalSection(&mCookieAccessCS);
}

int LoaderMgr::StoreCache(const char* url, int mime, int encoding, bool gzip)
{
	uint8 type;
	uint8 enc = (uint8)encoding;
	int cacheId;

	switch (mime) {
	case NEMIME_DOC_WMLC:
		type = NECACHET_WMLC;
		break;

	case NEMIME_DOC_WML:
	case NEMIME_DOC_XHTML:
	case NEMIME_DOC_HTML:
		type = (gzip) ? NECACHET_HTM_GZ : NECACHET_HTM;
		break;

	case NEMIME_DOC_CSS:
		type = (gzip) ? NECACHET_CSS_GZ : NECACHET_CSS;
		break;

	case NEMIME_IMAGE_GIF:
		type = NECACHET_GIF;
		break;

	case NEMIME_IMAGE_JPEG:
		type = NECACHET_JPG;
		break;

	case NEMIME_IMAGE_PNG:
		type = NECACHET_PNG;
		break;

	default:
		type = NECACHET_UNKNOWN;
	}

	EnterCriticalSection(&mCacheAccessCS);
	cacheId = cacheMgr_store(mCacheMgr, url, type, enc);
	LeaveCriticalSection(&mCacheAccessCS);

	return cacheId;
}

void LoaderMgr::SaveCacheData(int cacheId, char* data, int length, bool over)
{
	if (cacheId == -1)
		return;

	EnterCriticalSection(&mCacheAccessCS);
	cacheMgr_putData(mCacheMgr, cacheId, data, length, over);
	LeaveCriticalSection(&mCacheAccessCS);
}

int LoaderMgr::FindCache(const char* url, int* mime, int* encoding, int* length, bool* gzip)
{
	uint8 type;
	uint8 enc;
	int cacheId;

	EnterCriticalSection(&mCacheAccessCS);
	cacheId = cacheMgr_find(mCacheMgr, url, &type, &enc, length);
	LeaveCriticalSection(&mCacheAccessCS);

	if (cacheId == -1)
		return -1;

	*encoding = enc;
	*gzip = false;

	switch (type) {
	case NECACHET_WMLC:
		*mime = NEMIME_DOC_WMLC;
		break;

	case NECACHET_HTM:
		*mime = NEMIME_DOC_HTML;
		break;

	case NECACHET_HTM_GZ:
		*mime = NEMIME_DOC_HTML;
		*gzip = true;
		break;

	case NECACHET_CSS:
		*mime = NEMIME_DOC_CSS;
		break;

	case NECACHET_CSS_GZ:
		*mime = NEMIME_DOC_CSS;
		*gzip = true;
		break;

	case NECACHET_JS:
		break;

	case NECACHET_JS_GZ:
		break;

	case NECACHET_GIF:
		*mime = NEMIME_IMAGE_GIF;
		break;

	case NECACHET_JPG:
		*mime = NEMIME_IMAGE_JPEG;
		break;

	case NECACHET_PNG:
		*mime = NEMIME_IMAGE_PNG;
		break;

	default:
		*mime = NEMIME_UNKNOWN;
	}

	return cacheId;
}

int LoaderMgr::LoadCacheData(int cacheId, char* buf, int size, int from)
{
	int read;

	if (cacheId == -1)
		return -1;

	EnterCriticalSection(&mCacheAccessCS);
	read = cacheMgr_getData(mCacheMgr, cacheId, buf, size, from);
	LeaveCriticalSection(&mCacheAccessCS);

	return read;
}

bool LoaderMgr::IsPathExist(const char* path)
{
	int ret = _access(path, 0);
	return (ret == 0) ? true : false;
}

void LoaderMgr::MakeAllDir(const char* path)
{
	int ret;
	char dir[128];
	char* p;
	char deli;

	nbk_strcpy(dir, path);

	if (*(dir+1) == ':')
		p = dir + 3; // 跳过盘符
	else if (*dir == '\\' || *dir == '/')
		p = dir + 1; // 跳过根
	else
		p = dir;

	while (*p) {
		if (*p == '\\' || *p == '/') {
			deli = *p;
			*p = 0;
			ret = _mkdir(dir);
			*p = deli;
		}
		p++;
	}
}

void LoaderMgr::RemoveDir(const char* path)
{
	int err;
	intptr_t handle;
	struct _finddata_t data;

	if (_chdir(path) != 0)
		return;

	handle = _findfirst("*.*", &data);
	if (handle != -1) {
		do {
			if (!(data.attrib & _A_SUBDIR))
				err = _unlink(data.name);
		} while (_findnext(handle, &data) == 0);
		err = _findclose(handle);
		err = _chdir("..");
		err = _rmdir(path);
	}
}

void LoaderMgr::RemoveMultiDir(const char* path, const char* match)
{
	int err;
	intptr_t handle;
	struct _finddata_t data;

	if (_chdir(path) != 0)
		return;

	handle = _findfirst(match, &data);
	if (handle != -1) {
		do {
			if (data.attrib & _A_SUBDIR)
				RemoveDir(data.name);
		} while (_findnext(handle, &data) == 0);
		err = _findclose(handle);
	}
}

void LoaderMgr::GetHistoryPath(int pageId, char* path)
{
	sprintf(path, "c:\\nbkcache\\h%04d\\", pageId);
}
