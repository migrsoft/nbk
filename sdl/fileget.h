#ifndef __NBK_FILEGET_H__
#define __NBK_FILEGET_H__

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_timer.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NEFgetMode {
	NEFGET_NORMAL,
	NEFGET_FSCACHE
};

enum NEFileEventType {
	NFILE_EVENT_NONE,
	NFILE_EVENT_GOT_RESPONSE,
	NFILE_EVENT_GOT_DATA,
	NFILE_EVENT_COMPLETE,
	NFILE_EVENT_ERROR,
	NFILE_EVENT_LAST
};

typedef void (*FILE_EVENT_HANDLER)(void* fget, int evt, char* data, int len, void* user);

typedef struct _NFileGet {

	int		id;
	char*	path;

	nid		mode;

	nid		mime;
	nbool	gzip : 1;
	nbool	pkg : 1;

	FILE*	fd;
	int		fscId;
	NTimer*	t;

	int		state;
	int		length; // 文件长度
	int		read;
	char*	buf;

	FILE_EVENT_HANDLER	handler;
	void*	user;

} NFileGet;

NFileGet* fileGet_create(int id);
void fileGet_delete(NFileGet** fget);

void fileGet_setWorkMode(NFileGet* fget, nid mode);
void fileGet_setEventHandler(NFileGet* fget, FILE_EVENT_HANDLER handler, void* user);
void fileGet_start(NFileGet* fget, char* path);
void fileGet_cancel(NFileGet* fget);

#ifdef __cplusplus
}
#endif

#endif
