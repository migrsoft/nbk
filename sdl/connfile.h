#ifndef __NBK_CONN_FILE__
#define __NBK_CONN_FILE__

#include "resmgr.h"
#include "fileget.h"
#include "../stdc/loader/ngzip.h"
#include "../stdc/loader/pkgParser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NBK_fileConn {

	NBK_conn	d;

	char*		path;
	NFileGet*	fget;

	FILE*		cache;

    NPkgParser*	pkgPar; // CBSÊý¾Ý½âÎöÆ÷

} NBK_fileConn;

NBK_fileConn* fileConn_create(NBK_Request* request);
void fileConn_delete(NBK_fileConn** fileConn);

#ifdef __cplusplus
}
#endif

#endif
