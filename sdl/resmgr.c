// create by wuyulun, 2011.12.25

#include "../stdc/inc/nbk_limit.h"
#include "../stdc/inc/nbk_version.h"
#include "../stdc/loader/loader.h"
#include "../stdc/loader/url.h"
#include "../stdc/dom/history.h"
#include "../stdc/tools/str.h"
#include "resmgr.h"
#include "nbk.h"
#include "connfile.h"
#include "connhttp.h"
#include "md5.h"
#include "ini.h"
#include "runtime.h"

#define PAGE_ID_ALL	0xffff

#define MAX_CBS_URL_LEN	256

static int l_conn_id = 0;
static NBK_resMgr* l_resMgr = N_NULL;

nbool NBK_handleError(int error)
{
	return N_FALSE;
}

char* NBK_helper_getUserInfo(void)
{
    return resMgr_getUserInfo();
}

char* NBK_getStockPage(int type)
{
    switch (type) {
    case NESPAGE_ERROR: // 通用错误页
		return (char*)ini_getString(NEINI_PAGE_ERROR);
		break;

    case NESPAGE_ERROR_404: // 404 not found 错误页
		return (char*)ini_getString(NEINI_PAGE_404);
		break;

    default:
        return N_NULL;
    }
}

void NBK_resourceStopAll(void* pfd, nid pageId)
{
	//fprintf(stderr, "stop all resource belong to page %d\n", pageId);
	resMgr_deleteAllConn(l_resMgr, pageId, N_FALSE);
}

void NBK_resourceClean(void* pfd, nid pageId)
{
	char path[MAX_CACHE_PATH_LEN];
	sprintf(path, CACHE_PATH_FMT, ini_getString(NEINI_DATA_PATH), pageId);
	nbk_removeDir(path);
}

void conn_delete(NBK_conn* conn)
{
	loader_deleteRequest(&conn->request);
}

nbool conn_isMain(NBK_conn* conn)
{
	return (conn->request->type == NEREQT_MAINDOC);
}

nid conn_type(NBK_conn* conn)
{
	return (nid)conn->request->type;
}

nid conn_via(NBK_conn* conn)
{
	return (nid)conn->request->via;
}

nid conn_method(NBK_conn* conn)
{
	return (nid)conn->request->method;
}

char* conn_url(NBK_conn* conn)
{
	return conn->request->url;
}

nid conn_pageId(NBK_conn* conn)
{
	return conn->request->pageId;
}

/* 历史缓存 */

static void history_delete_dir(nid pageId)
{
	char* path = (char*)NBK_malloc(MAX_CACHE_PATH_LEN);
	sprintf(path, CACHE_PATH_FMT, ini_getString(NEINI_DATA_PATH), pageId);
	if (nbk_pathExist(path))
		nbk_removeDir(path);
	NBK_free(path);
}

char* historyCache_getFileName(NBK_conn* conn, int mime, nbool zip)
{
	char* path = (char*)NBK_malloc(MAX_CACHE_PATH_LEN);
	uint8 sig[16];
	char sigstr[34];

	if (   conn_via(conn) == NEREV_FF_FULL
		|| conn_via(conn) == NEREV_FF_NARROW )
	{
		sprintf(path, CACHE_FFP_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId);
	}
	else if (conn_via(conn) == NEREV_FF_MULTIPICS) {
		sprintf(path, CACHE_RES_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId);
	}
	else if (	conn_isMain(conn)
			 && (   mime == NEMIME_DOC_HTML
				 || mime == NEMIME_DOC_XHTML
				 || mime == NEMIME_DOC_WML ) )
	{
		if (zip)
			sprintf(path, CACHE_DOC_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId, DOC_GZIP_SUFFIX);
		else
			sprintf(path, CACHE_DOC_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId, DOC_HTML_SUFFIX);
	}
	else if (mime == NEMIME_DOC_CSS) {
		NBK_md5(conn->request->url, nbk_strlen(conn->request->url), sig);
		md5_sig_to_string(sig, sigstr, 32);
		sigstr[32] = 0;
		if (zip)
			sprintf(path, CACHE_CSS_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId, sigstr, CSS_GZIP_SUFFIX);
		else
			sprintf(path, CACHE_CSS_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId, sigstr, CSS_SUFFIX);
	}
	else if (   mime == NEMIME_IMAGE_JPEG
			 || mime == NEMIME_IMAGE_PNG
			 || mime == NEMIME_IMAGE_GIF )
	{
		NBK_md5(conn->request->url, nbk_strlen(conn->request->url), sig);
		md5_sig_to_string(sig, sigstr, 32);
		sigstr[32] = 0;
		sprintf(path, CACHE_PIC_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId, sigstr);
	}
	else {
		NBK_free(path);
		path = N_NULL;
	}

	return path;
}

void historyCache_open(NBK_conn* conn, FILE** fd, int mime, nbool zip)
{
	char* path = (char*)NBK_malloc(MAX_CACHE_PATH_LEN);

	historyCache_close(fd);

	if (conn_isMain(conn))
		history_delete_dir(conn->request->pageId);

	sprintf(path, CACHE_PATH_FMT, ini_getString(NEINI_DATA_PATH), conn->request->pageId);
	if (!nbk_pathExist(path))
		nbk_makeDir(path);
	NBK_free(path);

	path = historyCache_getFileName(conn, mime, zip);
	if (path) {
		*fd = fopen(path, "wb");
		NBK_free(path);
	}
}

void historyCache_write(FILE* fd, char* data, int len)
{
	if (fd) {
		fwrite(data, 1, len, fd);
	}
}

void historyCache_close(FILE** fd)
{
	if (*fd) {
		fclose(*fd);
		*fd = N_NULL;
	}
}

void resCache_open(FILE** fd, char* uri, nid pageId)
{
    char* path = (char*)NBK_malloc(MAX_CACHE_PATH_LEN + NBK_MAX_URL_LEN);
    char* name;
    int i;

	i = sprintf(path, CACHE_PATH_FMT, ini_getString(NEINI_DATA_PATH), pageId);

    name = path + i;

    nbk_strcpy(name, uri);
    for (i=0; name[i]; i++) {
        if (name[i] == ':')
            name[i] = '-';
        else if (name[i] == '/')
            name[i] = '_';
    }

    *fd = fopen(path, "wb");
    NBK_free(path);
}

//////////////////////////////////////////////////
//
// 资源管理器
//

// 清理已完成的连接
static void resMgr_schedule(void* user)
{
	NBK_resMgr* mgr = (NBK_resMgr*)user;
	NBK_conn* c;

	tim_stop(mgr->t);

	c = (NBK_conn*)sll_first(mgr->connLst);
    while (c) {
        if (c->state == CONN_FINISH) {
			if (c->type == CONN_FILE) {
				NBK_fileConn* fc = (NBK_fileConn*)c;
				fileConn_delete(&fc);
			}
			else if (c->type == CONN_HTTP) {
				NBK_httpConn* hc = (NBK_httpConn*)c;
				httpConn_delete(&hc);
			}
			sll_removeCurr(mgr->connLst);
        }
		c = (NBK_conn*)sll_next(mgr->connLst);
    }
}

static void resMgr_clean_conn(NBK_resMgr* mgr)
{
	tim_start(mgr->t, 10, 10);
}

static void load_wap_white_list(const char* fname)
{
    FILE* fd;
    int size;
    uint8* buf;

    fd = fopen(fname, "rb");
    if (fd == N_NULL)
        return;

    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    if (size > 20) {
        fseek(fd, 0, SEEK_SET);
        buf = (uint8*)NBK_malloc(size);
        fread(buf, 1, size, fd);
        url_parseWapWhiteList(buf, size);
        NBK_free(buf);
    }
    fclose(fd);
}

NBK_resMgr* resMgr_create(void)
{
	NBK_resMgr* mgr = (NBK_resMgr*)NBK_malloc0(sizeof(NBK_resMgr));
	char path[MAX_CACHE_PATH_LEN];

	l_resMgr = mgr;

	mgr->connLst = sll_create();
	mgr->t = tim_create(resMgr_schedule, mgr);

    mgr->mutex = SDL_CreateMutex();

	if (!nbk_pathExist(ini_getString(NEINI_DATA_PATH)))
		nbk_makeDir(ini_getString(NEINI_DATA_PATH));

	// 创建cookie管理器
	mgr->cookieMgr = cookieMgr_create();
	sprintf(path, COOKIE_PATH_FMT, ini_getString(NEINI_DATA_PATH));
	cookieMgr_load(mgr->cookieMgr, path);

	// 创建文件缓存管理器
	mgr->cacheMgr = cacheMgr_create();

    // 载入wap白名单
	sprintf(path, WAPWL_PATH_FMT, ini_getString(NEINI_DATA_PATH));
    load_wap_white_list(path);

	return mgr;
}

void resMgr_delete(NBK_resMgr** resMgr)
{
	NBK_resMgr* mgr = *resMgr;
	char path[MAX_CACHE_PATH_LEN];
    int i;
    NBK_conn* c;

	tim_delete(&mgr->t);

    // 取消所有下载
    c = (NBK_conn*)sll_first(mgr->connLst);
    while (c) {
        c->cancel(c);
        c = (NBK_conn*)sll_next(mgr->connLst);
    }

    // 等待线程结束
    for (i = 0; i < MAX_CONN_THREAD; i++) {
        if (mgr->threadList[i]) {
            fprintf(stderr, "waiting connection thread (%d) finish...\n", SDL_GetThreadID(mgr->threadList[i]));
            SDL_WaitThread(mgr->threadList[i], NULL);
        }
    }

	resMgr_deleteAllConn(mgr, PAGE_ID_ALL, N_TRUE);
	sll_delete(&mgr->connLst);

	sprintf(path, COOKIE_PATH_FMT, ini_getString(NEINI_DATA_PATH));
	cookieMgr_save(mgr->cookieMgr, path);
	cookieMgr_delete(&mgr->cookieMgr);

	cacheMgr_delete(&mgr->cacheMgr);

	if (mgr->bdua)
		NBK_free(mgr->bdua);
	if (mgr->addrCbs)
		NBK_free(mgr->addrCbs);
	if (mgr->addrMpic)
		NBK_free(mgr->addrMpic);
    if (mgr->userInfo)
        NBK_free(mgr->userInfo);

    SDL_DestroyMutex(mgr->mutex);

	NBK_free(mgr);
	*resMgr = N_NULL;
	l_resMgr = N_NULL;

    // 清除历史缓存目录
	nbk_removeMultiDir(ini_getString(NEINI_DATA_PATH), "cp*");

    // 清除云浏览会话信息
    for (i=1; ;i++) {
        sprintf(path, SESSION_FMT, ini_getString(NEINI_DATA_PATH), i);
        if (nbk_pathExist(path))
            nbk_removeFile(path);
        else
            break;
    }
}

NBK_resMgr* resMgr_getPtr(void)
{
	return l_resMgr;
}

NCookieMgr* resMgr_getCookieMgr(void)
{
	if (l_resMgr)
		return l_resMgr->cookieMgr;
	else
		return N_NULL;
}

NCacheMgr* resMgr_getCacheMgr(void)
{
	if (l_resMgr)
		return l_resMgr->cacheMgr;
	else
		return N_NULL;
}

void resMgr_onReceiveHeader(NBK_conn* conn, NRespHeader* header)
{
	if (conn_isMain(conn) && ini_getInt(NEINI_DUMP_DOC)) {
		char path[MAX_CACHE_PATH_LEN];
		sprintf(path, "%sdoc.htm", ini_getString(NEINI_DATA_PATH));
		dump_file_init(path, N_FALSE);
	}

	loader_onReceiveHeader(conn->request, header);
}

void resMgr_onReceiveData(NBK_conn* conn, char* data, int v1, int v2)
{
	if (conn_isMain(conn) && ini_getInt(NEINI_DUMP_DOC)) {
		char path[MAX_CACHE_PATH_LEN];
		sprintf(path, "%sdoc.htm", ini_getString(NEINI_DATA_PATH));
		dump_file_data(path, data, v1);
	}

	loader_onReceiveData(conn->request, data, v1, v2);
}

void resMgr_onComplete(NBK_conn* conn)
{
	loader_onComplete(conn->request);
	resMgr_clean_conn(l_resMgr);
}

void resMgr_onCancel(NBK_conn* conn)
{
	loader_onCancel(conn->request);
	resMgr_clean_conn(l_resMgr);
}

void resMgr_onError(NBK_conn* conn, int err)
{
	loader_onError(conn->request, err);
	resMgr_clean_conn(l_resMgr);
}

void resMgr_onPackageData(NBK_Request* req, NEResponseType type, void* data, int v1, int v2)
{
	if (ini_getInt(NEINI_DUMP_DOC)) {
		char path[MAX_CACHE_PATH_LEN];

		switch (type) {
		case NEREST_PKG_DATA_HEADER:
		case NEREST_PKG_DATA:
			sprintf(path, "%sdoc%02d.htm", ini_getString(NEINI_DATA_PATH), req->idx);
			break;
		}

		switch (type) {
		case NEREST_PKG_DATA_HEADER:
			dump_file_init(path, N_FALSE);
			break;
		case NEREST_PKG_DATA:
			dump_file_data(path, data, v1);
			break;
		}
	}

    loader_onPackage(req, type, data, v1, v2);
}

void resMgr_onPackageImage(NBK_Request* req, NEResponseType type, void* data, int len)
{
    switch (type) {
    case NEREST_HEADER:
        loader_onReceiveHeader(req, (NRespHeader*)data);
        break;
    case NEREST_DATA:
        loader_onReceiveData(req, (char*)data, len, len);
        break;
    case NEREST_COMPLETE:
        loader_onComplete(req);
        break;
	case NEREST_ERROR:
		loader_onError(req, NELERR_ERROR);
		break;
    }
}

void resMgr_onPackageCookie(int id, void* data, int len)
{
	char path[MAX_CACHE_PATH_LEN];

	if (id == 1) {
		// 获得新cookie，清理上次所有cookie
		int i = 1;
		while (1) {
			sprintf(path, SESSION_FMT, ini_getString(NEINI_DATA_PATH), i);
			if (nbk_pathExist(path))
				nbk_removeFile(path);
			else
				break;
			i++;
		}
	}

	sprintf(path, SESSION_FMT, ini_getString(NEINI_DATA_PATH), id);
	if (!nbk_pathExist(path))
		dump_file_init(path, N_FALSE);
	dump_file_data(path, data, len);
}

void resMgr_onPackageOver(NBK_conn* conn)
{
	loader_onPackageOver(conn->request);
	resMgr_clean_conn(l_resMgr);
}

int resMgr_onSaveImageHdr(const char* url, const NPkgImgInfo* info)
{
	return -1;
}

void resMgr_onSaveImageData(int idx, const uint8* data, int len)
{
}

void resMgr_onSaveImageOver(int idx)
{
}

static void resMgr_addConn(NBK_resMgr* resMgr, NBK_conn* conn)
{
    conn->id = ++l_conn_id;
	sll_append(resMgr->connLst, conn);
}

static void request_url_add_prefix(NBK_Request* req, const char* prefix)
{
    char* url;
    int size = 4;

    size += nbk_strlen(prefix);
    size += nbk_strlen(req->url);
    url = (char*)NBK_malloc(size);
    sprintf(url, "%s%s", prefix, req->url);

    loader_setRequestUrl(req, url, N_TRUE);
    req->via = NEREV_STANDARD;
}

nbool NBK_resourceLoad(void* pfd, NBK_Request* req)
{
	NBK_core* nbk = (NBK_core*)pfd;
	NBK_conn* c;

    if (req->via == NEREV_TF) {
        request_url_add_prefix(req, ini_getString(NEINI_TF));
    }

	//fprintf(stderr, "load resource %s\n", req->url);

	if (   req->via == NEREV_FF_FULL
        || req->via == NEREV_FF_NARROW
        || req->via == NEREV_FF_MULTIPICS
		|| req->method == NEREM_HISTORY )
	{
		NBK_httpConn* hc = httpConn_create(req);
		c = (NBK_conn*)hc;
		resMgr_addConn(nbk->resMgr, c);
		c->submit(c);
		return N_TRUE;
	}
	else {
		if (nbk_strncmp(req->url, "file://", 7) == 0) {
			NBK_fileConn* fc = fileConn_create(req);
			c = (NBK_conn*)fc;
			resMgr_addConn(nbk->resMgr, c);
			c->submit(c);
			return N_TRUE;
		}
		else if (nbk_strncmp(req->url, "http://", 7) == 0) {
			NBK_httpConn* hc = httpConn_create(req);
			c = (NBK_conn*)hc;
			resMgr_addConn(nbk->resMgr, c);
			c->submit(c);
			return N_TRUE;
		}
	}

	return N_FALSE;
}

void resMgr_deleteAllConn(NBK_resMgr* resMgr, nid pageId, nbool force)
{
	NBK_conn* c = (NBK_conn*)sll_first(resMgr->connLst);
    while (c) {
		if (pageId == PAGE_ID_ALL || c->request->pageId == pageId) {

			c->cancel(c);

			if (force || c->state == CONN_FINISH) {
				if (c->type == CONN_FILE) {
					NBK_fileConn* fc = (NBK_fileConn*)c;
					fileConn_delete(&fc);
				}
				else if (c->type == CONN_HTTP) {
					NBK_httpConn* hc = (NBK_httpConn*)c;
					httpConn_delete(&hc);
				}

				sll_removeCurr(resMgr->connLst);
			}
        }
		c = (NBK_conn*)sll_next(resMgr->connLst);
    }
}

static const char* get_uid(void)
{
	return "uid=nbk_0000000005";
}

static char* get_bdua(void)
{
	if (l_resMgr->bdua == N_NULL) {
		l_resMgr->bdua = (char*)NBK_malloc(64);
		sprintf(l_resMgr->bdua, "ua=nbk_%d_%d_win32_%d-%d-%d-%d_%s&from=%s",
			ini_getInt(NEINI_WIDTH), ini_getInt(NEINI_HEIGHT),
			NBK_VER_MAJOR, NBK_VER_MINOR, NBK_VER_REV, NBK_VER_BUILD,
			ini_getString(NEINI_PLATFORM), ini_getString(NEINI_FROM));
	}

	return l_resMgr->bdua;
}

char* resMgr_getCbsAddr(void)
{
	if (l_resMgr->addrCbs == N_NULL) {
		const char* addr = ini_getString(NEINI_CBS);
		l_resMgr->addrCbs = (char*)NBK_malloc(MAX_CBS_URL_LEN);
		sprintf(l_resMgr->addrCbs, "%s?%s&%s", addr, get_uid(), get_bdua());
	}

	return l_resMgr->addrCbs;
}

char* resMgr_getMpicAddr(void)
{
	if (l_resMgr->addrMpic == N_NULL) {
		const char* addr = ini_getString(NEINI_MPIC);
        const char* quality = ini_getString(NEINI_IMAGE_QUALITY);
		l_resMgr->addrMpic = (char*)NBK_malloc(MAX_CBS_URL_LEN);
		sprintf(l_resMgr->addrMpic, "%s&%s&%s&tc-quality=%s", addr, get_uid(), get_bdua(), quality);
	}

	return l_resMgr->addrMpic;
}

char* resMgr_getUserInfo(void)
{
    if (l_resMgr->userInfo == N_NULL) {
        l_resMgr->userInfo = (char*)NBK_malloc(MAX_CBS_URL_LEN);
        sprintf(l_resMgr->userInfo, "%s\r\nua=nbk_%d_%d_win32_%d-%d-%d-%d_%s\r\nfrom=%s\r\n",
                get_uid(),
			    ini_getInt(NEINI_WIDTH), ini_getInt(NEINI_HEIGHT),
			    NBK_VER_MAJOR, NBK_VER_MINOR, NBK_VER_REV, NBK_VER_BUILD,
			    ini_getString(NEINI_PLATFORM),
                ini_getString(NEINI_FROM));
    }

    return l_resMgr->userInfo;
}

uint8* resMgr_getCbsSession(int id, int* len)
{
	uint8* se = N_NULL;
	char path[MAX_CACHE_PATH_LEN];
	sprintf(path, SESSION_FMT, ini_getString(NEINI_DATA_PATH), id);
	if (nbk_pathExist(path)) {
		FILE* fd = fopen(path, "rb");
		if (fd) {
			int size;
			fseek(fd, 0, SEEK_END);
			size = ftell(fd);
			fseek(fd, 0, SEEK_SET);
			se = (uint8*)NBK_malloc(size);
			fread(se, 1, size, fd);
			fclose(fd);
			*len = size;
		}
	}
	return se;
}

void resMgr_addThread(SDL_Thread* thread)
{
    int i;

    SDL_LockMutex(l_resMgr->mutex);

    for (i = 0; i < MAX_CONN_THREAD; i++) {
        if (l_resMgr->threadList[i] == N_NULL) {
            l_resMgr->threadList[i] = thread;
            break;
        }
    }

    SDL_UnlockMutex(l_resMgr->mutex);
}

void resMgr_removeThread(SDL_Thread* thread)
{
    int i;

    SDL_LockMutex(l_resMgr->mutex);

    for (i = 0; i < MAX_CONN_THREAD; i++) {
        if (l_resMgr->threadList[i] == thread) {
            l_resMgr->threadList[i] = N_NULL;
            break;
        }
    }

    SDL_UnlockMutex(l_resMgr->mutex);
}
