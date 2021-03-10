#ifndef __CSSMGR_H__
#define __CSSMGR_H__

#include "../inc/config.h"
#include "../inc/nbk_limit.h"
#include "../tools/strBuf.h"
#include "../tools/smartPtr.h"

#ifdef __cplusplus
extern "C" {
#endif

// CSS manager

typedef struct _NExtCss {
    char*       url;
    NStrBuf*    dat;
    uint8       state;
} NExtCss;

typedef struct _NExtCssMgr {
	NSmartPtr	_smart;

	NExtCss     d[MAX_EXT_CSS_NUM];
	int16       cur;
	int16       use;
	NStrBuf*    cached; // inner css
	void*		doc;
	nbool		ready;
} NExtCssMgr;

NExtCssMgr* extCssMgr_create(void* doc);
void extCssMgr_delete(NExtCssMgr** mgr);

nbool extCssMgr_addExtCss(NExtCssMgr* mgr, char* url);
void extCssMgr_addInnCss(NExtCssMgr* mgr, const char* style, int length);
nbool extCssMgr_get(NExtCssMgr* mgr, char** style, int* len, nbool begin);
void extCssMgr_schedule(NExtCssMgr* mgr);
nbool extCssMgr_ready(NExtCssMgr* mgr);
void extCssMgr_stop(NExtCssMgr* mgr);

#ifdef __cplusplus
}
#endif

#endif // __CSSMGR_H__
