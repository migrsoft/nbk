#ifndef __NBK_COOKIEMGR_H__
#define __NBK_COOKIEMGR_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NCookie {
	char*	host;
	char*	path;
	char*	name;
	char*	value;
	uint32	expires;
	int		lru;
} NCookie;

typedef struct _NCookieMgr {
	NCookie**	lst;
} NCookieMgr;

NCookieMgr* cookieMgr_create(void);
void cookieMgr_delete(NCookieMgr** mgr);

void cookieMgr_setCookie(NCookieMgr* mgr, const char* host, char* cookie);
char* cookieMgr_getCookie(NCookieMgr* mgr, const char* host);

void cookieMgr_load(NCookieMgr* mgr, const char* fname);
void cookieMgr_save(NCookieMgr* mgr, const char* fname);

#ifdef __cplusplus
}
#endif

#endif
