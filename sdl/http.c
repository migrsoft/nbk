// create by wuyulun, 2012.1.10

#include "../stdc/inc/nbk_limit.h"
#include "../stdc/loader/url.h"
#include "../stdc/tools/str.h"
#include "http.h"
#include "ini.h"
#include "resmgr.h"

#define BUFFER_SIZE		16383

#define SOCKET_ERR_OPEN		1
#define SOCKET_ERR_CANCEL	5

static char* http_header_key[] = {
	"",
	"Connection",
	"Accept",
	"Accept-Charset",
	"Accept-Encoding",
	"Content-Length",
	"Content-Type",
	"Content-Encoding",
	"Referer",
	"Cookie",
	"User-Agent",
	0L
};

static char* http_keep_alive = "keep-alive";
static char* http_accept = "*/*";
static char* http_accept_charset = "GBK,utf-8;";
static char* http_useragent = "Palm/WebOS NBK/1.0";

NHttp* http_create(int id)
{
	NHttp* h = (NHttp*)NBK_malloc0(sizeof(NHttp));

	h->id = id;

	http_setHeader(h, HTTP_KEY_CONNECTION, http_keep_alive);
	http_setHeader(h, HTTP_KEY_ACCEPT, http_accept);
	http_setHeader(h, HTTP_KEY_ACCEPT_CHARSET, http_accept_charset);
	http_setHeader(h, HTTP_KEY_USERAGENT, http_useragent);

	h->buf = (char*)NBK_malloc(BUFFER_SIZE + 1);
    h->url = (char*)NBK_malloc(NBK_MAX_URL_LEN);
	h->length = -1;

	return h;
}

void http_delete(NHttp** http)
{
	NHttp* h = *http;

	if (h->host)
		NBK_free(h->host);
	if (h->response)
		NBK_free(h->response);
	if (h->responseStr)
		NBK_free(h->responseStr);
	if (h->redirectUrl)
		NBK_free(h->redirectUrl);

	NBK_free(h->buf);
    NBK_free(h->url);
	NBK_free(h);
	*http = N_NULL;
}

void http_setHeader(NHttp* http, int key, char* value)
{
	if (key > HTTP_KEY_BEGIN && key < HTTP_KEY_LAST)
		http->header[key] = value;
}

char* http_getHeader(NHttp* http, int key)
{
	if (key > HTTP_KEY_BEGIN && key < HTTP_KEY_LAST)
		return http->header[key];
	else
		return N_NULL;
}

char* http_getResponseHdrVal(NHttp* http, const char* hdr, int index)
{
	int i, idx;

	if (http->response) {
		for (i = idx = 0; http->response[i]; i+=2) {
			if (nbk_strcmp(http->response[i], hdr) == 0) {
				idx++;
				if (idx == index)
					return http->response[i+1];
			}
		}
	}

	return N_NULL;
}

int http_getContentLength(NHttp* http)
{
	if (http->chunked)
		return -1;
	else
		return http->length;
}

void http_setEventHandler(NHttp* http, HTTP_EVENT_HANDLER handler, void* user)
{
	http->handler = handler;
	http->user = user;
}

void http_setBodyProvider(NHttp* http, HTTP_BODY_PROVIDER provider, void* user)
{
	http->bodyProvider = provider;
	http->user = user;
}

static nbool create_socket(NHttp* http, char* url)
{
	int err;
	uint16 port;
	char* host = url_parseHostPort(url, &port);

	if (http->host) {
		NBK_free(http->host);
		http->host = N_NULL;
	}
	http->host = host;

	if (host) {
		err = SDLNet_ResolveHost(&http->ipaddr, host, port);
		if (err)
			return N_FALSE;
		http->socket = SDLNet_TCP_Open(&http->ipaddr);
		if (http->socket)
			return N_TRUE;
	}

	return N_FALSE;
}

// 获取路径
static char* url_get_path(char* url)
{
	int pos = nbk_strfind(url, "://");
	if (pos > 0) {
		char* p = url + pos + 3;
		while (*p && *p != '/' && *p != '?')
			p++;
		if (*p)
			return p;
	}
	return N_NULL;
}

static char* create_header(NHttp* http, char* url, int* length)
{
	char* blank = "";
	int size = 32;
	int i, pos, len;
	char* hdr;
	char clen[16];
	char* path;

	if (http->length > 0) {
		sprintf(clen, "%d", http->length);
		http->header[HTTP_KEY_CONTENT_LENGTH] = clen;
	}

	// GET url HTTP 1.1\r\n
	size += nbk_strlen(url) + 2;

	// Host: url\r\n
	size += nbk_strlen(http->host) + 8;

	for (i = HTTP_KEY_BEGIN + 1; i < HTTP_KEY_LAST; i++) {
		if (http->header[i]) {
			// key: value\r\n
			size += nbk_strlen(http_header_key[i]) + nbk_strlen(http->header[i]) + 4;
		}
	}

	hdr = (char*)NBK_malloc(size);

	path = url_get_path(url);
	if (path == N_NULL)
		path = blank;
	else if (*path == '/')
		path++;

	if (http->get)
		pos = sprintf(hdr, "GET /%s HTTP/1.1\r\n", path);
	else
		pos = sprintf(hdr, "POST /%s HTTP/1.1\r\n", path);

	len = sprintf(hdr + pos, "Host: %s\r\n", http->host);
	pos += len;

	for (i = HTTP_KEY_BEGIN + 1; i < HTTP_KEY_LAST; i++) {
		if (http->header[i]) {
			len = sprintf(hdr + pos, "%s: %s\r\n", http_header_key[i], http->header[i]);
			pos += len;
		}
	}

	nbk_strcpy(hdr + pos, "\r\n");
	pos += 2;
	*length = pos;

	http->header[HTTP_KEY_CONTENT_LENGTH] = N_NULL;
	http->header[HTTP_KEY_CONTENT_TYPE] = N_NULL;

	return hdr;
}

static char** parse_response_header(char* response, int* status, int id)
{
	int dbg = ini_getInt(NEINI_DBG_HTTP);
	char** pairs = N_NULL;
	char *p, *q;
	int i = 0;

	// 解析回应码
	p = response;
	while (*p != ' ')
		p++;
	p++;
	*status = NBK_atoi(p);

	while (*p != '\n')
		p++;
	p++;

	// 统计头字段总数
	q = p;
	while (*p) {
		if (*p == '\r')
			i++;
		p++;
	}

	pairs = (char**)NBK_malloc0(sizeof(char*) * (i * 2));

	// 解析头字段
	i = 0;
	p = q;
	while (*p) {
		if (*p == '\r') {
			*p = 0;
			i++;
		}
		else if (*p == '\n')
			*p = 0;
		else if (*p == ':' && i % 2 == 0) {
			*p = 0;
			i++;
		}
		else if (pairs[i] == N_NULL && *p != ' ')
			pairs[i] = p;
		p++;
	}

	if (dbg)
		fprintf(stderr, "http (%d) code %d\n", id, *status);
	for (i=0; pairs[i]; i+=2) {
		str_toLower(pairs[i], -1);
		if (dbg)
			fprintf(stderr, "%s [ %s ]\n", pairs[i], pairs[i+1]);
	}

	return pairs;
}

static void http_notify(NHttp* http, int evt, char* data, int len)
{
	if (evt == HTTP_EVENT_CONNECT_CLOSED || !http->stop)
		http->handler(http, evt, data, len, http->user);
}

static int http_transfer(void* data)
{
	int dbg = ini_getInt(NEINI_DBG_HTTP);
	NHttp* http = (NHttp*)data;
	char* reqHdr;
	int hdrLen;
	int bytes, offs;
	char *p, *q, *toofar;
	int crlf;
	int i;
	nbool quit = N_FALSE;
	nbool stop = N_FALSE;
	int err = 0;
    int received = 0;
    Uint32 __time;
    nbool __time_got;

	while (!stop && http->times < 5) {
		if (create_socket(http, http->url)) {

            received = 0;

			// 发送消息头
			reqHdr = create_header(http, http->url, &hdrLen);
			if (dbg) {
				fprintf(stderr, "\n---------- http request ----- (%d)(%d) ----->>>\n", http->id, http->times);
				fprintf(stderr, "%s", reqHdr);
			}
			bytes = SDLNet_TCP_Send(http->socket, reqHdr, hdrLen);
			NBK_free(reqHdr);
			if (bytes != hdrLen) {
				err = 11;
				break;
			}

			// 发送消息体
			if (!http->get && http->length > 0 && http->bodyProvider) {
				offs = 0;
				while (http->length > 0) {
					int read;
					http->bodyProvider(offs, http->buf, BUFFER_SIZE, &read, http->user);
					bytes = SDLNet_TCP_Send(http->socket, http->buf, read);
					if (bytes != read) {
						err = 12;
						break;
					}
					offs += bytes;
					http->length -= bytes;
				}
				if (err)
					break;
			}

            __time = SDL_GetTicks();
            __time_got = N_FALSE;

			// 接收回应

			if (http->responseStr)
				NBK_free(http->responseStr);
			if (http->response)
				NBK_free(http->response);
			http->responseStr = N_NULL;
			http->response = N_NULL;
			hdrLen = 0;
			http->status = -1;
			http->length = -1;
			http->chunked = N_FALSE;

			offs = 0;
			while (!http->stop && !quit) {
				bytes = SDLNet_TCP_Recv(http->socket, &http->buf[offs], BUFFER_SIZE - offs);
                if (!__time_got) {
                    __time_got = N_TRUE;
                    fprintf(stderr, "Server response in %.3f seconds\n", (double)(SDL_GetTicks() - __time) / 1000);
                }
                received += bytes;
				if (bytes <= 0) {
					err = 21;
					break;
				}
				if (http->stop) {
					err = SOCKET_ERR_CANCEL;
					break;
				}

				bytes += offs;
				http->buf[bytes] = 0;
				p = http->buf;
				toofar = p + bytes;
				offs = 0;

				if (http->status == -1) { // 回应头未完成
					q = N_NULL; // 记录分隔符位置
					crlf = 0;
					while (p < toofar) {
						if (*p == '\r')
							q = p;
						else if (*p == '\n' && p - 1 == q)
							crlf++;
						else {
							q = N_NULL;
							crlf = 0;
						}
						p++;

						if (crlf == 2) { // 回应头完整
							if (hdrLen == 0) {
								hdrLen = p - http->buf;
								http->responseStr = (char*)NBK_malloc(hdrLen + 1);
								NBK_memcpy(http->responseStr, http->buf, hdrLen);
							}
							else {
								q = http->responseStr + hdrLen;
								hdrLen += p - http->buf;
								http->responseStr = (char*)NBK_realloc(http->responseStr, hdrLen + 1);
								NBK_memcpy(q, http->buf, p - http->buf);
							}
							http->responseStr[hdrLen] = 0;
							if (dbg) {
								fprintf(stderr, "<<<---------- http response ----- (%d)(%d) -----\n", http->id, http->times);
								//fprintf(stderr, "%s", http->responseStr);
								fprintf(stderr, "REQ %s\n", http->url);
							}
							http->response = parse_response_header(http->responseStr, &http->status, http->id);

							for (i=0; http->response[i]; i+=2) {
								if (   nbk_strcmp(http->response[i], "transfer-encoding") == 0
									&& nbk_strcmp(http->response[i+1], "chunked") == 0 )
									http->chunked = N_TRUE;
								else if (nbk_strcmp(http->response[i], "content-length") == 0)
									http->length = NBK_atoi(http->response[i+1]);
							}

							if (http->status < 300 || http->status >= 400)
								http_notify(http, HTTP_EVENT_GOT_RESPONSE, N_NULL, 0);
							break;
						}
					}

					if (http->status == -1) {
						p = (q) ? q : p; // 数据分隔位置

						if (hdrLen == 0) {
							hdrLen = p - http->buf;
							http->responseStr = (char*)NBK_malloc(hdrLen + 1);
							NBK_memcpy(http->responseStr, http->buf, hdrLen);
						}
						else {
							q = http->responseStr + hdrLen;
							hdrLen += p - http->buf;
							http->responseStr = (char*)NBK_realloc(http->responseStr, hdrLen + 1);
							NBK_memcpy(q, http->buf, p - http->buf);
						}

						offs = toofar - p;
						if (offs > 0)
							NBK_memcpy(http->buf, p, offs); // 移动未处理数据至缓冲区首部
					}
				}

				if (http->status == -1)
					continue;
                if (http->status >= 300 && http->status < 400)
					break;

				// 接收消息体
				while (p < toofar) {
					if (http->stop) {
						err = SOCKET_ERR_CANCEL;
						break;
					}

					if (http->chunked) {
						if (http->length == -1) {
							q = p;
							while (q < toofar - 1 && *q != '\r')
								q++;
							if (q == toofar - 1) {
								offs = toofar - p;
								NBK_memcpy(http->buf, p, offs);
								p = toofar;
							}
							else {
								*q = 0;
								q += 2;
								http->length = NBK_htoi(p); // 读取 chunk 数据长
								p = q;
							}
						}

						if (http->length == 0) {
							quit = N_TRUE;
							break;
						}

						bytes = toofar - p;
						if (bytes <= 0)
							break;

						if (bytes >= http->length) {
							if (dbg)
								fprintf(stderr, "http (%d) -> %d chunk OVER\n", http->id, http->length);
							http_notify(http, HTTP_EVENT_GOT_DATA, p, http->length);
							p += http->length + 2;
							http->length = -1;
						}
						else {
							if (dbg)
								fprintf(stderr, "http (%d) -> %d\n", http->id, bytes);
							http_notify(http, HTTP_EVENT_GOT_DATA, p, bytes);
							http->length -= bytes;
							p += bytes;
							if (http->length == 0)
								http->length = -1;
						}
					}
					else { // 指定长度
						bytes = toofar - p;
						if (bytes <= 0)
							break;

						if (http->length == -1 || bytes < http->length) {
							if (dbg)
								fprintf(stderr, "http (%d) -> %d\n", http->id, bytes);
							http_notify(http, HTTP_EVENT_GOT_DATA, p, bytes);
							if (http->length > 0)
								http->length -= bytes;
						}
						else {
							if (dbg)
								fprintf(stderr, "http (%d) -> %d OVER\n", http->id, http->length);
							http_notify(http, HTTP_EVENT_GOT_DATA, p, http->length);
							http->length = 0;
						}

						if (http->length == 0)
							quit = N_TRUE;

						break;
					}
				}
			}

			SDLNet_TCP_Close(http->socket);
			http->socket = N_NULL;
			if (dbg)
				fprintf(stderr, "http (%d)(%d) socket close.\n", http->id, http->times);

			if (err)
				break;

			if (http->status >= 300 && http->status < 400) {
				char* url = http_getResponseHdrVal(http, "location", 1);
				if (url) {
					if (http->redirectUrl)
						NBK_free(http->redirectUrl);
					http->redirectUrl = url_parse(http->url, url);
					http->times++;
					http_notify(http, HTTP_EVENT_REDIRECT, N_NULL, 0);

					// 请求重定向地址
					http->get = N_TRUE;
					http_setHeader(http, HTTP_KEY_REFERER, http->url);
					nbk_strcpy(http->url, http->redirectUrl);
					http->length = -1;
				}
				else {
					stop = N_TRUE;
					err = 31;
				}
			}
			else {
				stop = N_TRUE;
				if (http->length == -1 && dbg)
					fprintf(stderr, "http (%d) -> OVER\n", http->id);
				http_notify(http, HTTP_EVENT_COMPLETE, N_NULL, 0);
			}
		}
		else {
			// 创建socket出错
			stop = N_TRUE;
			err = SOCKET_ERR_OPEN;
		}
	} // while

	if (err) {
		if (http->socket) {
			SDLNet_TCP_Close(http->socket);
			http->socket = N_NULL;
		}
		if (dbg)
			fprintf(stderr, "http (%d) ERROR %d received: %d\n", http->id, err, received);
		http_notify(http, (received > 0) ? HTTP_EVENT_COMPLETE : HTTP_EVENT_ERROR, N_NULL, 0);
	}

	if (dbg)
		fprintf(stderr, "http (%d) transcation close\n", http->id);

	http_notify(http, HTTP_EVENT_CONNECT_CLOSED, N_NULL, 0);

    resMgr_removeThread(http->worker);
	return 0;
}

nbool http_get(NHttp* http, char* url)
{
	N_ASSERT(http->handler);

	http->get = N_TRUE;
	nbk_strcpy(http->url, url);
	http->length = -1;
	http->worker = SDL_CreateThread(http_transfer, http);
    resMgr_addThread(http->worker);
	return N_TRUE;
}

nbool http_post(NHttp* http, char* url, int bodyLen)
{
	N_ASSERT(http->handler);

	http->get = N_FALSE;
	nbk_strcpy(http->url, url);
	http->length = bodyLen;
	http->worker = SDL_CreateThread(http_transfer, http);
    resMgr_addThread(http->worker);
	return N_TRUE;
}

void http_cancel(NHttp* http)
{
	http->stop = N_TRUE;

	if (ini_getInt(NEINI_DBG_HTTP))
		fprintf(stderr, "http (%d) cancel\n", http->id);
}
