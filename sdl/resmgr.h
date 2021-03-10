#ifndef _NBK_RESMGR_H_
#define _NBK_RESMGR_H_

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_timer.h"
#include "../stdc/inc/nbk_gdi.h"
#include "../stdc/loader/loader.h"
#include "../stdc/loader/pkgParser.h"
#include "../stdc/tools/slist.h"
#include <stdio.h>
#include <SDL.h>
#include "cookiemgr.h"
#include "cache.h"
#include "probe.h"

#define MAX_CACHE_PATH_LEN	128
#define MAX_CONN_THREAD     8

#define CACHE_PATH_FMT	"%scp%05d/"
#define CACHE_DOC_FMT	"%scp%05d/doc%s"
#define CACHE_FFP_FMT	"%scp%05d/ff.cbs" // 页面数据包
#define CACHE_RES_FMT	"%scp%05d/res.cbs" // 图片数据包
#define CACHE_CSS_FMT	"%scp%05d/%s%s"
#define CACHE_PIC_FMT	"%scp%05d/%s.pic"
#define COOKIE_PATH_FMT	"%scookie.dat"
#define SESSION_FMT		"%sse%03d"
#define WAPWL_PATH_FMT  "%snbk_wl.dat"

#define DOC_HTML_SUFFIX	".htm"
#define DOC_GZIP_SUFFIX	".gz"
#define CSS_SUFFIX		".css"
#define CSS_GZIP_SUFFIX	".csz"
#define	IMG_JPG_SUFFIX	".jpg"
#define IMG_PNG_SUFFIX	".png"
#define IMG_GIF_SUFFIX	".gif"

#ifdef __cplusplus
extern "C" {
#endif

enum EConnType {
	CONN_NONE,
	CONN_FILE,
	CONN_HTTP
};

enum EConnState {
	CONN_INIT,
	CONN_WAIT,
	CONN_ACTIVE,
	CONN_FINISH
};

typedef struct _NBK_conn {

	int		id; // 连接惟一标识
	int		type;
	int		state;

	NBK_Request*	request;

	void (*submit)(struct _NBK_conn*);
	void (*cancel)(struct _NBK_conn*);

} NBK_conn;

typedef struct _NBK_resMgr {

	NSList*		connLst;
	NTimer*		t;

	NCookieMgr*	cookieMgr;
	NCacheMgr*	cacheMgr;

	char*	bdua; // 百度UA
	char*	addrCbs; // 云浏览请求地址
	char*	addrMpic; // 多图请求地址
    char*   userInfo;

	NBK_probe*	probe;

    SDL_Thread* threadList[MAX_CONN_THREAD];
    SDL_mutex*  mutex; // 用于记录线程使用

} NBK_resMgr;

// 历史缓存
char* historyCache_getFileName(NBK_conn* conn, int mime, nbool zip);
void historyCache_open(NBK_conn* conn, FILE** fd, int mime, nbool zip);
void historyCache_write(FILE* fd, char* data, int len);
void historyCache_close(FILE** fd);

// 资源缓存
void resCache_open(FILE** fd, char* uri, nid pageId);

// 基础连接接口
void conn_delete(NBK_conn* conn);
nbool conn_isMain(NBK_conn* conn);
nid conn_type(NBK_conn* conn);
nid conn_via(NBK_conn* conn);
nid conn_method(NBK_conn* conn);
char* conn_url(NBK_conn* conn);
nid conn_pageId(NBK_conn* conn);

// -----------------------------------------------------------------------------
// 资源管理器
// -----------------------------------------------------------------------------

NBK_resMgr* resMgr_create(void);
void resMgr_delete(NBK_resMgr** resMgr);

NBK_resMgr* resMgr_getPtr(void);
NCookieMgr* resMgr_getCookieMgr(void);
NCacheMgr* resMgr_getCacheMgr(void);

// 记录线程使用
void resMgr_addThread(SDL_Thread* thread);
void resMgr_removeThread(SDL_Thread* thread);

// 标准请求回调
void resMgr_onReceiveHeader(NBK_conn* conn, NRespHeader* header);
void resMgr_onReceiveData(NBK_conn* conn, char* data, int v1, int v2);
void resMgr_onComplete(NBK_conn* conn);
void resMgr_onCancel(NBK_conn* conn);
void resMgr_onError(NBK_conn* conn, int err);

// CBS请求回调
void resMgr_onPackageData(NBK_Request* req, NEResponseType type, void* data, int v1, int v2);
void resMgr_onPackageImage(NBK_Request* req, NEResponseType type, void* data, int len);
void resMgr_onPackageCookie(int id, void* data, int len);
void resMgr_onPackageOver(NBK_conn* conn);

int  resMgr_onSaveImageHdr(const char* url, const NPkgImgInfo* info);
void resMgr_onSaveImageData(int idx, const uint8* data, int len);
void resMgr_onSaveImageOver(int idx);

void resMgr_deleteAllConn(NBK_resMgr* resMgr, nid pageId, nbool force);

char* resMgr_getCbsAddr(void);
char* resMgr_getMpicAddr(void);
char* resMgr_getUserInfo(void);

uint8* resMgr_getCbsSession(int id, int* len);

#ifdef __cplusplus
}
#endif

#endif
