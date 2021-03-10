#ifndef __NBK_CONN_HTTP__
#define __NBK_CONN_HTTP__

#include "resmgr.h"
#include "http.h"
#include "fileget.h"
#include "../stdc/loader/ngzip.h"
#include "../stdc/loader/pkgParser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NBK_httpConn {

	NBK_conn	d;

	char*		cookie;
	char*		contentType;
	NHttp*		http;
	NBK_gzip*	ngz;

	char*		path;
	NFileGet*	fget;

	FILE*		historyCache;	// 历史缓存
	int			cacheId;		// 文件缓存

	NPkgParser*	pkgPar; // CBS数据解析器

	uint8*		deflatedBody;
	int			deflatedBodyLen;

} NBK_httpConn;

NBK_httpConn* httpConn_create(NBK_Request* request);
void httpConn_delete(NBK_httpConn** conn);

#ifdef __cplusplus
}
#endif

#endif
