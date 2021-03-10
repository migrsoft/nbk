// create by wuyulun, 2012.1.10

#include "../stdc/inc/nbk_limit.h"
#include "../stdc/dom/history.h"
#include "../stdc/loader/loader.h"
#include "../stdc/loader/url.h"
#include "../stdc/tools/str.h"
#include "nbk.h"
#include "runtime.h"
#include "connhttp.h"
#include "resmgr.h"
#include "cookiemgr.h"
#include "cache.h"

static char* http_accept_encoding = "gzip, deflate";
static char* http_content_encoding_gzip = "gzip";
static char* form_urlencoded = "application/x-www-form-urlencoded";
static char* form_octet_stream = "application/octet-stream";

static void http_event_complete(NBK_httpConn* conn)
{
	NCacheMgr* cache = resMgr_getCacheMgr();

	if (conn->cacheId != -1)
		cacheMgr_saveOver(cache, conn->cacheId, N_TRUE);

    historyCache_close(&conn->historyCache);

	if (conn->ngz) {
		ngzip_closeUnzip(conn->ngz);
		ngzip_delete(&conn->ngz);
	}

	if (conn->pkgPar)
		resMgr_onPackageOver(&conn->d);
	else
		resMgr_onComplete(&conn->d);
}

static void http_event_error(NBK_httpConn* conn, int error)
{
	NCacheMgr* cache = resMgr_getCacheMgr();

	if (conn->cacheId != -1)
		cacheMgr_saveOver(cache, conn->cacheId, N_FALSE);

    historyCache_close(&conn->historyCache);

	if (conn->ngz) {
		ngzip_closeUnzip(conn->ngz);
		ngzip_delete(&conn->ngz);
	}

	if (conn->pkgPar)
		resMgr_onPackageOver(&conn->d);
	else
		resMgr_onError(&conn->d, error);
}

static void cookie_collect(NHttp* http)
{
	NCookieMgr* mgr = resMgr_getCookieMgr();
	char* cookie;
	int index = 1;

	if (mgr == N_NULL)
		return;

	do {
		cookie = http_getResponseHdrVal(http, "set-cookie", index);
		index++;
		if (cookie) {
			cookieMgr_setCookie(mgr, http->host, cookie);
		}
	} while (cookie);
}

static char* cookie_get(char* url)
{
	NCookieMgr* mgr = resMgr_getCookieMgr();
	int i;
	char *p, *q, save;
	char* cookie;

	if (mgr == N_NULL)
		return N_NULL;

	i = nbk_strfind(url, "://");
	if (i == -1)
		return N_NULL;

	p = url + i + 3;
	q = p;
	save = 0;
	while (*q && *q != '/') q++;
	if (*q) save = *q;
	*q = 0;

	cookie = cookieMgr_getCookie(mgr, p);
	*q = save;
	
	return cookie;
}

// 网络数据处理
static void http_event_handler(void* http, int evt, char* data, int len, void* user)
{
	NHttp* h = (NHttp*)http;
	NBK_httpConn* c = (NBK_httpConn*)user;
    NBK_conn* nc = (NBK_conn*)c;
	NCacheMgr* cache = resMgr_getCacheMgr();
    nbool isImage = N_FALSE;

	sync_wait("Http");
	//fprintf(stderr, "HTTP (%d) event --->\n", h->id);

	switch (evt) {
	case HTTP_EVENT_CONNECT_CLOSED:
	{
		// 联网线程退出，连接可结束
		c->d.state = CONN_FINISH;
		break;
	}
	case HTTP_EVENT_REDIRECT:
	{
		c->d.request->via = NEREV_STANDARD;

		cookie_collect(h);
		if (c->cookie)
			NBK_free(c->cookie);
		c->cookie = cookie_get(h->redirectUrl);
		http_setHeader(h, HTTP_KEY_COOKIE, c->cookie);
		break;
	}
	case HTTP_EVENT_GOT_RESPONSE:
	{
		char* hdrVal;
		NRespHeader hdr;

		NBK_memset(&hdr, 0, sizeof(NRespHeader));

		// 发生重定向后，置换为最终URL
		if (h->redirectUrl) {
			char* url = (char*)NBK_malloc(nbk_strlen(h->redirectUrl) + 1);
			nbk_strcpy(url, h->redirectUrl);
			loader_setRequestUrl(c->d.request, url, N_TRUE);
		}

		// 获取长度
		hdr.content_length = http_getContentLength(h);

		// 检测回应类型
		hdrVal = http_getResponseHdrVal(h, "content-type", 1);
		if (hdrVal) {
            // 检测类型
			if (nbk_strfind(hdrVal, "html") != -1)
				hdr.mime = NEMIME_DOC_HTML;
			else if (nbk_strfind(hdrVal, "wap.wmlc") != -1)
				hdr.mime = NEMIME_DOC_WMLC;
			else if (nbk_strfind(hdrVal, "wap") != -1)
				hdr.mime = NEMIME_DOC_WML;
			else if (nbk_strfind(hdrVal, "xhtml") != -1)
				hdr.mime = NEMIME_DOC_XHTML;
			else if (nbk_strfind(hdrVal, "css") != -1)
				hdr.mime = NEMIME_DOC_CSS;
			else if (nbk_strfind(hdrVal, "jpg") != -1 || nbk_strfind(hdrVal, "jpeg") != -1)
				hdr.mime = NEMIME_IMAGE_JPEG;
			else if (nbk_strfind(hdrVal, "png") != -1)
				hdr.mime = NEMIME_IMAGE_PNG;
			else if (nbk_strfind(hdrVal, "gif") != -1)
				hdr.mime = NEMIME_IMAGE_GIF;

            if (   hdr.mime == NEMIME_IMAGE_JPEG
                || hdr.mime == NEMIME_IMAGE_PNG
                || hdr.mime == NEMIME_IMAGE_GIF )
                isImage = N_TRUE;

            // 检测编码
            if (   nbk_strfind(hdrVal, "gb2312") != -1
                || nbk_strfind(hdrVal, "gbk") != -1 )
                hdr.encoding = NEENC_GB2312;
		}

		// 当服务器返回wml压缩包时，取消内部数据包解析器，转入标准流程
		if ((hdr.mime == NEMIME_DOC_WML || hdr.mime == NEMIME_DOC_WMLC) && c->pkgPar) {
			c->d.request->via = NEREV_STANDARD;
			pkgParser_delete(&c->pkgPar);
		}

		if (conn_via(&c->d) == NEREV_STANDARD) {
			hdrVal = http_getResponseHdrVal(h, "content-encoding", 1);
			if (hdrVal) {
				if (nbk_strfind(hdrVal, "gzip") != -1)
					c->ngz = ngzip_create();
			}
		}

		if ( cache &&
			(  hdr.mime == NEMIME_DOC_CSS
			|| hdr.mime == NEMIME_IMAGE_JPEG
			|| hdr.mime == NEMIME_IMAGE_PNG
			|| hdr.mime == NEMIME_IMAGE_GIF ) )
		{
			// 创建长期文件缓存
			nid type = NECACHET_UNKNOWN;
			if (hdr.mime == NEMIME_DOC_CSS)
				type = (c->ngz) ? NECACHET_CSS_GZ : NECACHET_CSS;
			else if (hdr.mime == NEMIME_IMAGE_JPEG)
				type = NECACHET_JPG;
			else if (hdr.mime == NEMIME_IMAGE_PNG)
				type = NECACHET_PNG;
			else if (hdr.mime == NEMIME_IMAGE_GIF)
				type = NECACHET_GIF;

			c->cacheId = cacheMgr_save(cache, c->d.request->url, type);
            resCache_open(&c->historyCache, c->d.request->url, c->d.request->pageId);
		}
		else {
			// 创建历史缓存，仅在程序运行中存在，用于前进后退操作
			historyCache_open((NBK_conn*)c, &c->historyCache, hdr.mime, (c->ngz) ? N_TRUE : N_FALSE);
		}

		if (   conn_via(&c->d) != NEREV_FF_FULL
			&& conn_via(&c->d) != NEREV_FF_NARROW
			&& conn_via(&c->d) != NEREV_FF_MULTIPICS )
		{
            if ((hdr.mime == NEMIME_UNKNOWN || isImage) && conn_type(&c->d) == NEREQT_MAINDOC) {
                // 未知文件
                fprintf(stderr, "Attachment ==> %s\n", http_getResponseHdrVal(h, "content-type", 1));
                http_cancel(h);

                if (isImage) {
                    // 当请求主文档为图片时，将该图片包装成页面
                    int size = nbk_strlen(c->d.request->url);
                    char* html = (char*)NBK_malloc(size + 128);
                    size = sprintf(html, "<!doctype xhtml \"DTD/xhtml-mobile10.dtd\"><html><body><img src='%s'/>", c->d.request->url);
                    hdr.mime = NEMIME_DOC_XHTML;
                    hdr.content_length = size;
                    hdr.encoding = NEENC_UTF8;
        			resMgr_onReceiveHeader(nc, &hdr);
                    resMgr_onReceiveData(nc, html, size, size);
                    resMgr_onComplete(nc);
                    historyCache_open(nc, &c->historyCache, hdr.mime, N_FALSE);
                    historyCache_write(c->historyCache, html, size);
                    historyCache_close(&c->historyCache);
                    NBK_free(html);
                }
                break;
            }
			cookie_collect(h);
			resMgr_onReceiveHeader((NBK_conn*)c, &hdr);
		}
		break;
	}
	case HTTP_EVENT_GOT_DATA:
	{
		if (c->cacheId != -1)
			cacheMgr_saveData(cache, c->cacheId, (uint8*)data, len);

		historyCache_write(c->historyCache, data, len);

		if (c->pkgPar) {
			pkgParser_processData(c->pkgPar, (uint8*)data, len);
		}
		else if (c->ngz) {
			uint8* unzip;
			ngzip_addZippedData(c->ngz, (uint8*)data, len);
			unzip = ngzip_getData(c->ngz, &len);
			if (unzip)
				resMgr_onReceiveData((NBK_conn*)c, (char*)unzip, len, len);
		}
		else {
			resMgr_onReceiveData((NBK_conn*)c, data, len, len);
		}

		break;
	}
	case HTTP_EVENT_COMPLETE:
	{
		http_event_complete(c);
		break;
	}
	case HTTP_EVENT_ERROR:
		http_event_error(c, NELERR_ERROR);
		break;
	};

	//fprintf(stderr, "HTTP (%d) event <---\n", h->id);
	sync_post("Http");
}

// 本地数据处理
static void file_event_handler(void* fget, int evt, char* data, int len, void* user)
{
	NFileGet* f = (NFileGet*)fget;
	NBK_httpConn* c = (NBK_httpConn*)user;

	switch (evt) {
	case NFILE_EVENT_GOT_RESPONSE:
	{
		NRespHeader hdr;
		NBK_memset(&hdr, 0, sizeof(NRespHeader));

		hdr.mime = f->mime;
		hdr.content_length = f->length;

		if (f->gzip)
			c->ngz = ngzip_create();

		if (c->pkgPar == N_NULL)
			resMgr_onReceiveHeader((NBK_conn*)c, &hdr);
		break;
	}
	case NFILE_EVENT_GOT_DATA:
	{
		if (c->pkgPar) {
			pkgParser_processData(c->pkgPar, (uint8*)data, len);
		}
		else if (c->ngz) {
			uint8* unzip;
			ngzip_addZippedData(c->ngz, (uint8*)data, len);
			unzip = ngzip_getData(c->ngz, &len);
			if (unzip)
				resMgr_onReceiveData((NBK_conn*)c, (char*)unzip, len, len);
		}
		else
			resMgr_onReceiveData((NBK_conn*)c, data, len, len);
		break;
	}
	case NFILE_EVENT_COMPLETE:
	{
		c->d.state = CONN_FINISH;
		if (c->ngz) {
			ngzip_closeUnzip(c->ngz);
			ngzip_delete(&c->ngz);
		}
		if (c->pkgPar)
			resMgr_onPackageOver(&c->d);
		else
			resMgr_onComplete(&c->d);
		break;
	}
	case NFILE_EVENT_ERROR:
	{
		c->d.state = CONN_FINISH;
		if (c->ngz) {
			ngzip_closeUnzip(c->ngz);
			ngzip_delete(&c->ngz);
		}
		if (c->pkgPar)
			resMgr_onPackageOver(&c->d);
		else
			resMgr_onError(&c->d, NELERR_NOCACHE);
		break;
	}
	}
}

static nbool http_body_provider(int offset, char* buf, int bufLen, int* dataLen, void* user)
{
	NBK_httpConn* hc = (NBK_httpConn*)user;
	NBK_conn* nc = (NBK_conn*)user;
	uint8* body;
	int len, size;

	if (hc->deflatedBody) {
		len = hc->deflatedBodyLen;
		body = hc->deflatedBody;
	}
	else if (nc->request->body) {
		len = nbk_strlen(nc->request->body);
		body = (uint8*)nc->request->body;
	}
	else {
		len = nc->request->upcmd->cur;
		body = nc->request->upcmd->buf;
	}

	size = len - offset;
	if (size > bufLen)
		size = bufLen;

	NBK_memcpy(buf, body + offset, size);
	*dataLen = size;

	return N_TRUE;
}

static void httpConn_create_pkgparser(NBK_httpConn* conn)
{
	conn->pkgPar = pkgParser_create(conn->d.request);
    conn->pkgPar->page = history_curr(g_nbk_core->history);

	// 挂接图片数据回调
	conn->pkgPar->datPar->cbOnData = resMgr_onPackageData;
	conn->pkgPar->datPar->cbOnImage = resMgr_onPackageImage;
	conn->pkgPar->datPar->cbOnCookie = resMgr_onPackageCookie;

	// 挂接图片缓存回调(fsc)
	conn->pkgPar->datPar->cbSaveImgHdr = resMgr_onSaveImageHdr;
	conn->pkgPar->datPar->cbSaveImgData = resMgr_onSaveImageData;
	conn->pkgPar->datPar->cbSaveImgOver = resMgr_onSaveImageOver;
}

static nid httpConn_genPath(NBK_httpConn* conn, nbool* zip)
{
	NBK_conn* nc = (NBK_conn*)conn;
	char* path = N_NULL;

	if (conn_via(nc) == NEREV_FF_MULTIPICS) {
		path = historyCache_getFileName(nc, 0, N_FALSE);
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			httpConn_create_pkgparser(conn);
			*zip = N_FALSE;
			return 0;
		}
	}
	else if (conn_isMain(nc)) {
		// 检测页面数据包
		uint8 via = nc->request->via;
		nc->request->via = NEREV_FF_FULL;
		path = historyCache_getFileName(nc, 0, N_FALSE);
		nc->request->via = via;
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			httpConn_create_pkgparser(conn);
			*zip = N_FALSE;
			return 0;
		}
		if (path) NBK_free(path);

		// 检测未压缩页面
		path = historyCache_getFileName(nc, NEMIME_DOC_HTML, N_FALSE);
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			*zip = N_FALSE;
			return NEMIME_DOC_HTML;
		}
		if (path) NBK_free(path);

		// 检测压缩页面
		path = historyCache_getFileName(nc, NEMIME_DOC_HTML, N_TRUE);
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			*zip = N_TRUE;
			return NEMIME_DOC_HTML;
		}
	}
	else if (conn_type(nc) == NEREQT_CSS) {
		// 检测未压缩样式
		path = historyCache_getFileName(nc, NEMIME_DOC_CSS, N_FALSE);
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			*zip = N_FALSE;
			return NEMIME_DOC_CSS;
		}
		if (path) NBK_free(path);

		// 检测压缩样式
		path = historyCache_getFileName(nc, NEMIME_DOC_CSS, N_TRUE);
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			*zip = N_TRUE;
			return NEMIME_DOC_CSS;
		}
	}
	else if (conn_type(nc) == NEREQT_IMAGE) {
		// 检测图片
		path = historyCache_getFileName(nc, NEMIME_IMAGE_JPEG, N_FALSE);
		if (path && nbk_pathExist(path)) {
			conn->path = path;
			*zip = N_FALSE;
			return NEMIME_IMAGE_JPEG;
		}
	}

	if (path) NBK_free(path);

	return NEMIME_UNKNOWN;
}

static void httpConn_reset(NBK_httpConn* conn)
{
	if (conn->fget)
		fileGet_delete(&conn->fget);
	if (conn->http)
		http_delete(&conn->http);
	if (conn->ngz) {
        ngzip_closeUnzip(conn->ngz);
		ngzip_delete(&conn->ngz);
    }
	if (conn->pkgPar)
		pkgParser_delete(&conn->pkgPar);

	if (conn->cookie) {
		NBK_free(conn->cookie);
		conn->cookie = N_NULL;
	}
	if (conn->contentType) {
		NBK_free(conn->contentType);
		conn->contentType = N_NULL;
	}
	if (conn->deflatedBody) {
		NBK_free(conn->deflatedBody);
		conn->deflatedBody = N_NULL;
	}
}

static void httpConn_add_cookie(NBK_httpConn* conn)
{
	int via = conn_via(&conn->d);

	if (   via == NEREV_FF_FULL
		|| via == NEREV_FF_NARROW
		|| via == NEREV_FF_MULTIPICS )
	{
		int i, lastSize, size;
		uint8* se;
		i = 1;
		lastSize = 0;
		while (1) {
			se = resMgr_getCbsSession(i, &size);
			if (se) {
				if (lastSize != size) {
					lastSize = size;
					upcmd_iaSession(conn->d.request->upcmd, se, size);
				}
				NBK_free(se);
				i++;
			}
			else
				break;
		}
	}
	else {
		conn->cookie = cookie_get(conn->d.request->url);
		http_setHeader(conn->http, HTTP_KEY_COOKIE, conn->cookie);
	}
}

static void httpConn_submit(NBK_conn* conn)
{
	NBK_httpConn* c = (NBK_httpConn*)conn;

	c->d.state = CONN_ACTIVE;
	c->cacheId = -1;

	httpConn_reset(c);

	if (   conn_type(conn) == NEREQT_CSS
		|| conn_type(conn) == NEREQT_IMAGE )
	{
		int cacheId;
		NCacheMgr* cache = resMgr_getCacheMgr();
		cacheId = cacheMgr_load(cache, conn->request->url);
		if (cacheId != -1) {
			c->fget = fileGet_create(conn->id);
			c->fget->fscId = cacheId;
			fileGet_setWorkMode(c->fget, NEFGET_FSCACHE);
			fileGet_setEventHandler(c->fget, file_event_handler, c);
			fileGet_start(c->fget, conn->request->url);
			return;
		}
		else {
			//fprintf(stderr, "\nMISS: %s\n", conn->request->url);
		}
	}

	if (conn_method(conn) == NEREM_HISTORY) {
		nbool zip;
		c->fget = fileGet_create(conn->id);
		c->fget->mime = httpConn_genPath(c, &zip);
		c->fget->gzip = zip;
		fileGet_setEventHandler(c->fget, file_event_handler, c);
		fileGet_start(c->fget, c->path);
	}
	else {
		nbool get = N_TRUE;
		char* reqUrl;
		int bodyLen = 0;

		c->http = http_create(conn->id);
		http_setHeader(c->http, HTTP_KEY_REFERER, conn->request->referer);
		http_setEventHandler(c->http, http_event_handler, c);

		httpConn_add_cookie(c);

		if (   conn_via(conn) == NEREV_FF_FULL
			|| conn_via(conn) == NEREV_FF_NARROW )
		{
			get = N_FALSE;
			reqUrl = resMgr_getCbsAddr();
			bodyLen = conn->request->upcmd->cur;
			http_setHeader(c->http, HTTP_KEY_CONTENT_TYPE, form_octet_stream);
			httpConn_create_pkgparser(c);
		}
		else if (conn_via(conn) == NEREV_FF_MULTIPICS) {
			// 压缩图片请求消息体
			uint8* dp;
			int dpLen;
			NBK_gzip* gz = ngzip_create();
			ngzip_addUnzippedData(gz, conn->request->upcmd->buf, conn->request->upcmd->cur);
			dp = ngzip_getData(gz, &dpLen);
			if (dp) {
				c->deflatedBodyLen = 10 + dpLen;
				c->deflatedBody = (uint8*)NBK_malloc(c->deflatedBodyLen);
				NBK_memcpy(c->deflatedBody, NGZ_HEADER, 10);
				NBK_memcpy(c->deflatedBody + 10, dp, dpLen);
				http_setHeader(c->http, HTTP_KEY_CONTENT_ENCODING, http_content_encoding_gzip);
			}
			ngzip_closeZip(gz);
			ngzip_delete(&gz);

			get = N_FALSE;
			reqUrl = resMgr_getMpicAddr();
			bodyLen = (c->deflatedBody) ? c->deflatedBodyLen : conn->request->upcmd->cur;
			http_setHeader(c->http, HTTP_KEY_CONTENT_TYPE, form_octet_stream);
			httpConn_create_pkgparser(c);
		}
		else { // 标准请求
			reqUrl = conn->request->url;

			// 对非图片请求启用压缩
			if (conn_type(conn) != NEREQT_IMAGE)
				http_setHeader(c->http, HTTP_KEY_ACCEPT_ENCODING, http_accept_encoding);

			if (conn->request->body) {
				get = N_FALSE;
				bodyLen = nbk_strlen(conn->request->body);

				if (conn->request->enc == NEENCT_URLENCODED) {
					http_setHeader(c->http, HTTP_KEY_CONTENT_TYPE, form_urlencoded);
				}
				else if (conn->request->enc == NEENCT_MULTIPART) {
					c->contentType = multipart_contentType();
					http_setHeader(c->http, HTTP_KEY_CONTENT_TYPE, c->contentType);
				}
			}
		}

		if (get) {
			http_get(c->http, reqUrl);
		}
		else {
			http_setBodyProvider(c->http, http_body_provider, c);
			http_post(c->http, reqUrl, bodyLen);
		}
	}
}

static void httpConn_cancel(NBK_conn* conn)
{
	NBK_httpConn* c = (NBK_httpConn*)conn;

	if (c->http)
		http_cancel(c->http);
	if (c->fget)
		fileGet_cancel(c->fget);

	historyCache_close(&c->historyCache);
}

NBK_httpConn* httpConn_create(NBK_Request* request)
{
	NBK_httpConn* c = (NBK_httpConn*)NBK_malloc0(sizeof(NBK_httpConn));
	c->d.type = CONN_HTTP;
	c->d.request = request;
	c->d.submit = httpConn_submit;
	c->d.cancel = httpConn_cancel;
	return c;
}

void httpConn_delete(NBK_httpConn** conn)
{
	NBK_httpConn* c = *conn;
	conn_delete(&c->d);

	httpConn_reset(c);

	if (c->path)
		NBK_free(c->path);

	NBK_free(c);
	*conn = N_NULL;
}
