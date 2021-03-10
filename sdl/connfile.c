// create by wuyulun, 2012.1.10

#include "../stdc/inc/nbk_limit.h"
#include "../stdc/tools/str.h"
#include "nbk.h"
#include "connfile.h"
#include "resmgr.h"
#include "ini.h"

//////////////////////////////////////////////////
//
// 本地资源
//

static void fileConn_parse_path(NBK_fileConn* conn)
{
    const char* wp = get_work_path();
	char *p = conn->d.request->url + 7; // 跳过协议名

    conn->path = (char*)NBK_malloc(MAX_FILE_PATH);
    if (*p == '/')
        nbk_strcpy(conn->path, p);
    else
        sprintf(conn->path, "%s/%s", wp, p);
}

static void file_event_handler(void* fget, int evt, char* data, int len, void* user)
{
	NFileGet* f = (NFileGet*)fget;
	NBK_fileConn* c = (NBK_fileConn*)user;

	switch (evt) {
	case NFILE_EVENT_GOT_RESPONSE:
	{
		NRespHeader hdr;
		NBK_memset(&hdr, 0, sizeof(NRespHeader));

		hdr.mime = f->mime;
		hdr.content_length = f->length;

        if (c->pkgPar == N_NULL) {
		    historyCache_open((NBK_conn*)c, &c->cache, hdr.mime, N_FALSE);
		    resMgr_onReceiveHeader((NBK_conn*)c, &hdr);
        }
		break;
	}
	case NFILE_EVENT_GOT_DATA:
	{
        if (c->pkgPar) {
			pkgParser_processData(c->pkgPar, (uint8*)data, len);
        }
        else {
		    historyCache_write(c->cache, data, len);
		    resMgr_onReceiveData((NBK_conn*)c, data, len, len);
        }
		break;
	}
	case NFILE_EVENT_COMPLETE:
	{
		c->d.state = CONN_FINISH;
        if (c->pkgPar) {
		    resMgr_onPackageOver((NBK_conn*)c);
        }
        else {
		    historyCache_close(&c->cache);
		    resMgr_onComplete((NBK_conn*)c);
        }
		break;
	}
	case NFILE_EVENT_ERROR:
	{
		c->d.state = CONN_FINISH;
        if (c->pkgPar) {
		    resMgr_onPackageOver((NBK_conn*)c);
        }
        else {
		    historyCache_close(&c->cache);
		    resMgr_onError((NBK_conn*)c, 404);
        }
		break;
	}
	}
}

static void fileConn_reset(NBK_fileConn* conn)
{
	if (conn->path) {
		NBK_free(conn->path);
        conn->path = N_NULL;
    }

    if (conn->fget)
		fileGet_delete(&conn->fget);

    if (conn->pkgPar)
        pkgParser_delete(&conn->pkgPar);
}

static void fileConn_create_pkgparser(NBK_fileConn* conn)
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

static void fileConn_submit(NBK_conn* conn)
{
	NBK_fileConn* c = (NBK_fileConn*)conn;
	c->d.state = CONN_ACTIVE;

    fileConn_reset(c);

	fileConn_parse_path(c);

    if (nbk_strfind(c->path, ".cbs") > 7) // 检测是否 cbs 包
        fileConn_create_pkgparser(c);

	c->fget = fileGet_create(conn->id);
	c->fget->mime = NEMIME_DOC_HTML;
	fileGet_setEventHandler(c->fget, file_event_handler, c);
	fileGet_start(c->fget, c->path);
}

static void fileConn_cancel(NBK_conn* conn)
{
	NBK_fileConn* c = (NBK_fileConn*)conn;
	c->d.state = CONN_FINISH;

	if (c->fget)
		fileGet_cancel(c->fget);

	historyCache_close(&c->cache);
}

NBK_fileConn* fileConn_create(NBK_Request* request)
{
	NBK_fileConn* c = (NBK_fileConn*)NBK_malloc0(sizeof(NBK_fileConn));
	c->d.type = CONN_FILE;
	c->d.request = request;
	c->d.submit = fileConn_submit;
	c->d.cancel = fileConn_cancel;
	return c;
}

void fileConn_delete(NBK_fileConn** fileConn)
{
	NBK_fileConn* c = *fileConn;
	conn_delete(&c->d);
    fileConn_reset(c);
	NBK_free(c);
	*fileConn = N_NULL;
}
