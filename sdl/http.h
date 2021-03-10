#ifndef __NBK_HTTP_H__
#define __NBK_HTTP_H__

#include "../stdc/inc/config.h"
#include <SDL.h>
#include <SDL_net.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NEHttpKey {
	HTTP_KEY_BEGIN,
	HTTP_KEY_CONNECTION,
	HTTP_KEY_ACCEPT,
	HTTP_KEY_ACCEPT_CHARSET,
	HTTP_KEY_ACCEPT_ENCODING,
	HTTP_KEY_CONTENT_LENGTH,
	HTTP_KEY_CONTENT_TYPE,
	HTTP_KEY_CONTENT_ENCODING,
	HTTP_KEY_REFERER,
	HTTP_KEY_COOKIE,
	HTTP_KEY_USERAGENT,
	HTTP_KEY_LAST
};

enum NEHttpEventType {
	HTTP_EVENT_NULL,
	HTTP_EVENT_GOT_RESPONSE,
	HTTP_EVENT_GOT_DATA,
	HTTP_EVENT_COMPLETE,
	HTTP_EVENT_ERROR,
	HTTP_EVENT_REDIRECT,
	HTTP_EVENT_CONNECT_CLOSED,
	HTTP_EVENT_LAST
};

typedef void (*HTTP_EVENT_HANDLER)(void* http, int evt, char* data, int len, void* user);
typedef nbool (*HTTP_BODY_PROVIDER)(int offset, char* buf, int bufLen, int* dataLen, void* user);

typedef struct _NHttp {

	int		id;

	IPaddress	ipaddr;
	TCPsocket	socket;
	SDL_Thread*	worker;

	char*	url;
	char*	host;
	int		status;
	int		length;
	int		times;

	nbool	get : 1;
	nbool	chunked : 1;
	nbool	stop : 1; // 停止传输

	char*	header[HTTP_KEY_LAST];
	char**	response; // 解析后的键值对
	char*	responseStr;
	char*	redirectUrl;

	char*	buf; // 接收数据缓存区

	HTTP_EVENT_HANDLER	handler;
	HTTP_BODY_PROVIDER	bodyProvider;
	void*	user;

} NHttp;

NHttp* http_create(int id);
void http_delete(NHttp** http);

void http_setHeader(NHttp* http, int key, char* value);
char* http_getHeader(NHttp* http, int key);
char* http_getResponseHdrVal(NHttp* http, const char* hdr, int index);
int http_getContentLength(NHttp* http);

void http_setEventHandler(NHttp* http, HTTP_EVENT_HANDLER handler, void* user);
void http_setBodyProvider(NHttp* http, HTTP_BODY_PROVIDER provider, void* user);

nbool http_get(NHttp* http, char* url);
nbool http_post(NHttp* http, char* url, int bodyLen);

void http_cancel(NHttp* http);

#ifdef __cplusplus
}
#endif

#endif
