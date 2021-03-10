#include "stdafx.h"
#include "NConnHttp.h"
#include "LoaderMgr.h"
#include "../../stdc/tools/str.h"
#include "../../stdc/loader/loader.h"

static char* http_accept_encoding = "gzip,deflate";
static char* http_set_cookie = "set-cookie";
static char* http_content_type = "content-type";
static char* http_cache_control = "cache-control";
static char* form_urlencoded = "application/x-www-form-urlencoded";
static char* form_octet_stream = "application/octet-stream";

static char* pic_htm = "<body><img src=\"%s\" /></body>";

NConnHttp::NConnHttp(int id, NRequest* req, LoaderMgr* loader) : NConn(id, req, loader)
{
	mHttp = new NHttp();
	mHttp->SetListener(this);
	mGzip = NULL;
	mCookie = NULL;
	mCacheId = -1;
	mPicPage = false;
	mBeRedirect = false;

	mWritePath = NULL;
	mWriteBuffer = NULL;
}

NConnHttp::~NConnHttp()
{
	if (mGzip) {
		ngzip_closeUnzip(mGzip);
		ngzip_delete(&mGzip);
	}

	ReplaceCookie(NULL);

	delete mHttp;

	if (mWritePath)
		NBK_free(mWritePath);
	if (mWriteBuffer)
		strBuf_delete(&mWriteBuffer);

	TCHAR msg[64];
	wsprintf(msg, _T("NConnHttp (%d) finish.\n"), GetId());
	OutputDebugString(msg);
}

void NConnHttp::Run()
{
	mWritePath = GetHistoryFileName();
	mWriteBuffer = strBuf_create();

	if (   mRequest->type == NEREQT_MAINDOC
		|| mRequest->type == NEREQT_SUBDOC
		|| mRequest->type == NEREQT_CSS )
		mHttp->SetHeader(NHttp::HTTP_KEY_ACCEPT_ENCODING, http_accept_encoding);

	// 加入 cookie
	mCookie = GetCookie(mRequest->url);
	mHttp->SetHeader(NHttp::HTTP_KEY_COOKIE, mCookie);

	mHttp->SetHeader(NHttp::HTTP_KEY_REFERER, mRequest->referer);

	mHttp->Get(mRequest->url);
}

void NConnHttp::OnHttpEvent(int evt, char* data, int len)
{
	switch (evt) {
	case NHttp::HTTP_EVENT_REDIRECT:
	{
		mBeRedirect = true;

		CollectCookie();

		char* c = mLoader->GetCookie(mHttp->GetRedirectUrl());
		ReplaceCookie(c);
		mHttp->SetHeader(NHttp::HTTP_KEY_COOKIE, mCookie);
		break;
	}

	case NHttp::HTTP_EVENT_GOT_RESPONSE:
	{
		CollectCookie();

		// 发生重定向，更新请求地址
		if (mHttp->GetRedirectUrl()) {
			char* u = str_clone(mHttp->GetRedirectUrl());
			loader_setRequestUrl(mRequest, u, N_TRUE);
		}

		if (mHttp->IsGzipped()) {
			mGzip = ngzip_create();
		}

		// 检测文档类型、编码
		char* val = mHttp->GetResponseHdrVal(http_content_type, 1);
		int mime = NEMIME_UNKNOWN;
		int encoding = NEENC_UNKNOWN;

		if (val) {
            // 检测类型
				 if (nbk_strfind(val, "xhtml") != -1)
				mime = NEMIME_DOC_XHTML;
			else if (nbk_strfind(val, "html") != -1)
				mime = NEMIME_DOC_HTML;
			else if (nbk_strfind(val, "wap.wmlc") != -1)
				mime = NEMIME_DOC_WMLC;
			else if (nbk_strfind(val, "wap") != -1)
				mime = NEMIME_DOC_WML;
			else if (nbk_strfind(val, "css") != -1)
				mime = NEMIME_DOC_CSS;
			else if (nbk_strfind(val, "gif") != -1)
				mime = NEMIME_IMAGE_GIF;
			else if (nbk_strfind(val, "jpg") != -1 || nbk_strfind(val, "jpeg") != -1)
				mime = NEMIME_IMAGE_JPEG;
			else if (nbk_strfind(val, "png") != -1)
				mime = NEMIME_IMAGE_PNG;

            // 检测编码
            if (nbk_strfind(val, "gb2312") != -1 || nbk_strfind(val, "gbk") != -1)
                encoding = NEENC_GB2312;
			else if (nbk_strfind(val, "utf-8") != -1)
				encoding = NEENC_UTF8;
		}

		// 是否缓存该资源
		bool cache = true;
		if (mBeRedirect) {
			// 当产生重向定时，最终地址与初始地址不一致，不进行缓存
			cache = false;
		}
		else {
			val = mHttp->GetResponseHdrVal(http_cache_control, 1);
			if (val && nbk_strfind(val, "no-cache") != -1)
				cache = false;
		}

		if (	cache
			&& (   mime == NEMIME_DOC_CSS
				|| mime == NEMIME_IMAGE_GIF
				|| mime == NEMIME_IMAGE_JPEG
				|| mime == NEMIME_IMAGE_PNG ) )
			mCacheId = mLoader->StoreCache(mRequest->url, mime, encoding, mHttp->IsGzipped());

		if (mRequest->type == NEREQT_MAINDOC && mime == NEMIME_IMAGE_JPEG) {
			// 当请求页面为图片数据时，产生临时页面
			mPicPage = true;
			mPicPageLength = nbk_strlen(pic_htm) + nbk_strlen(mRequest->url) - 2;
			ReceiveHeader(NEMIME_DOC_XHTML, NEENC_UTF8, mPicPageLength);
		}
		else
			ReceiveHeader(mime, encoding, mHttp->GetLength());
		break;
	}

	case NHttp::HTTP_EVENT_GOT_DATA:
	{
		mLoader->SaveCacheData(mCacheId, data, len, false);

		if (mPicPage) {
			char* d = (char*)NBK_malloc(mPicPageLength + 1);
			sprintf(d, pic_htm, mRequest->url);
			ReceiveData(d, mPicPageLength, mPicPageLength);
			break;
		}

		int uncomprLen = len;
		if (mGzip) {
			uint8* unzip;
			ngzip_addZippedData(mGzip, (uint8*)data, len);
			unzip = ngzip_getData(mGzip, &uncomprLen);
			data = (char*)unzip;
		}

		if (data) {
			char* d = (char*)NBK_malloc(uncomprLen);
			NBK_memcpy(d, data, uncomprLen);
			if (mWriteBuffer)
				strBuf_appendStr(mWriteBuffer, d, uncomprLen);
			ReceiveData(d, uncomprLen, len);
		}
		break;
	}

	case NHttp::HTTP_EVENT_COMPLETE:
		mLoader->SaveCacheData(mCacheId, NULL, 0, true);
		SaveHistoryResource();
		ReceiveComplete();
		break;

	case NHttp::HTTP_EVENT_ERROR:
		mLoader->SaveCacheData(mCacheId, NULL, 0, true);
		ReceiveError(-1);
		break;

	case NHttp::HTTP_EVENT_CONNECT_CLOSED:
		mLoader->SaveCacheData(mCacheId, NULL, 0, true);
		break;
	}
}

void NConnHttp::ReplaceCookie(char* cookie)
{
	if (mCookie)
		NBK_free(mCookie);
	mCookie = cookie;
}

void NConnHttp::CollectCookie()
{
	char* cookie;
	int i = 1;

	do {
		cookie = mHttp->GetResponseHdrVal(http_set_cookie, i++);
		if (cookie)
			mLoader->SetCookie(mHttp->GetHost(), cookie);
	} while (cookie);
}

char* NConnHttp::GetCookie(char* url)
{
	int i;
	char *p, *q, save;
	char* cookie;

	i = nbk_strfind(url, "://");
	if (i == -1)
		return N_NULL;

	p = url + i + 3;
	q = p;
	save = 0;
	while (*q && *q != '/') q++;
	if (*q) save = *q;
	*q = 0;

	cookie = mLoader->GetCookie(p);
	*q = save;
	
	return cookie;
}

void NConnHttp::SaveHistoryResource()
{
	char* data;
	int len = 0;
	FILE* f;

	if (mWriteBuffer) {
		strBuf_joinAllStr(mWriteBuffer);
		strBuf_getStr(mWriteBuffer, &data, &len, N_TRUE);

		if (len > 0) {
			f = fopen(mWritePath, "wb");
			if (f) {
				fwrite(data, 1, len, f);
				fclose(f);
			}
		}
	}
}
