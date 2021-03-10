#include "stdafx.h"
#include "NHttp.h"
#include "../../stdc/inc/config.h"
#include "../../stdc/inc/nbk_limit.h"
#include "../../stdc/tools/str.h"
#include "../../stdc/loader/url.h"

//#define DEBUG

static char* HTTP_HEADER_KEY[] = {
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
static char* http_useragent = "NBK/2.0";

NHttp::NHttp()
{
	mAddrinfo = NULL;
	mSocket = INVALID_SOCKET;

	NBK_memset(mHeaders, 0, sizeof(char*) * HTTP_KEY_LAST);
	SetHeader(HTTP_KEY_CONNECTION, http_keep_alive);
	SetHeader(HTTP_KEY_ACCEPT, http_accept);
	SetHeader(HTTP_KEY_ACCEPT_CHARSET, http_accept_charset);
	SetHeader(HTTP_KEY_USERAGENT, http_useragent);

	mUrl = (char*)NBK_malloc(NBK_MAX_URL_LEN);
	mReferer = NULL;
	mRedirectUrl = N_NULL;
	mHost = N_NULL;
	mLength = -1;
	mResponseStr = N_NULL;
	mResponse = N_NULL;

	mListener = N_NULL;
}

NHttp::~NHttp()
{
	if (mAddrinfo)
		freeaddrinfo(mAddrinfo);

	NBK_free(mUrl);
	if (mReferer)
		NBK_free(mReferer);
	if (mRedirectUrl)
		NBK_free(mRedirectUrl);
	if (mHost)
		NBK_free(mHost);
	if (mResponseStr)
		NBK_free(mResponseStr);
	if (mResponse)
		NBK_free(mResponse);
}

void NHttp::Start()
{
	int err = 0;
	bool stop = false;
	bool quit = false;
	int times = 0;
	char* reqHdr;
	int hdrLen;
	int bytes, offs, received = 0;
	char* buf = (char*)NBK_malloc(BUF_SIZE + 1);
	char *p, *q, *toofar;
	int crlf;

	while (!stop && times < 5) {
		if (CreateSocket()) {
			reqHdr = CreateHeaders();

#ifdef DEBUG
			printf("\n\nREQUEST: %s\n", mUrl);
			printf("%s", reqHdr);
#endif

			hdrLen = nbk_strlen(reqHdr);
			bytes = send(mSocket, reqHdr, hdrLen, 0);
			NBK_free(reqHdr);
			if (bytes != hdrLen) {
				err = 11;
				break;
			}

			if (mResponseStr)
				NBK_free(mResponseStr);
			if (mResponse)
				NBK_free(mResponse);

			hdrLen = 0;
			mStatus = -1;
			mLength = -1;
			mChunked = false;

			offs = 0;
			received = 0;
			while (!quit) {
				bytes = recv(mSocket, &buf[offs], BUF_SIZE - offs, 0);
				received += bytes;
				if (bytes <= 0) {
					err = 21;
					break;
				}

				bytes += offs;
				buf[bytes] = 0;
				p = buf;
				toofar = p + bytes;
				offs = 0;

				if (mStatus == -1) { // 回应头未完成
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
								hdrLen = p - buf;
								mResponseStr = (char*)NBK_malloc(hdrLen + 1);
								NBK_memcpy(mResponseStr, buf, hdrLen);
							}
							else {
								q = mResponseStr + hdrLen;
								hdrLen += p - buf;
								mResponseStr = (char*)NBK_realloc(mResponseStr, hdrLen + 1);
								NBK_memcpy(q, buf, p - buf);
							}
							mResponseStr[hdrLen] = 0;
							ParseResponse();

							for (int i=0; mResponse[i]; i+=2) {
								if (   nbk_strcmp(mResponse[i], "transfer-encoding") == 0
									&& nbk_strcmp(mResponse[i+1], "chunked") == 0 )
									mChunked = true;
								else if (nbk_strcmp(mResponse[i], "content-length") == 0)
									mLength = NBK_atoi(mResponse[i+1]);
								else if (nbk_strcmp(mResponse[i], "content-encoding") == 0) {
									if (nbk_strfind(mResponse[i+1], "gzip") != -1)
										mGzip = true;
								}
							}

							if (mStatus < 300 || mStatus >= 400)
								Notify(HTTP_EVENT_GOT_RESPONSE, N_NULL, 0);
							break;
						}
					}

					if (mStatus == -1) {
						p = (q) ? q : p; // 数据分隔位置

						if (hdrLen == 0) {
							hdrLen = p - buf;
							mResponseStr = (char*)NBK_malloc(hdrLen + 1);
							NBK_memcpy(mResponseStr, buf, hdrLen);
						}
						else {
							q = mResponseStr + hdrLen;
							hdrLen += p - buf;
							mResponseStr = (char*)NBK_realloc(mResponseStr, hdrLen + 1);
							NBK_memcpy(q, buf, p - buf);
						}

						offs = toofar - p;
						if (offs > 0)
							NBK_memcpy(buf, p, offs); // 移动未处理数据至缓冲区首部
					}
				}

				if (mStatus == -1)
					continue;
                if (mStatus >= 300 && mStatus < 400)
					break;

				// 接收消息体
				while (p < toofar) {

					if (mChunked) {
						if (mLength == -1) {
							q = p;
							while (q < toofar - 1 && *q != '\r')
								q++;
							if (q == toofar - 1) {
								offs = toofar - p;
								NBK_memcpy(buf, p, offs);
								p = toofar;
							}
							else {
								*q = 0;
								q += 2;
								mLength = nbk_htoi(p); // 读取 chunk 数据长
								p = q;
							}
						}

						if (mLength == 0) {
							quit = true;
							break;
						}

						bytes = toofar - p;
						if (bytes <= 0)
							break;

						if (bytes >= mLength) {
							Notify(HTTP_EVENT_GOT_DATA, p, mLength);
							p += mLength + 2;
							mLength = -1;
						}
						else {
							Notify(HTTP_EVENT_GOT_DATA, p, bytes);
							mLength -= bytes;
							p += bytes;
							if (mLength == 0)
								mLength = -1;
						}
					}
					else { // 指定长度
						bytes = toofar - p;
						if (bytes <= 0)
							break;

						if (mLength == -1 || bytes < mLength) {
							Notify(HTTP_EVENT_GOT_DATA, p, bytes);
							if (mLength > 0)
								mLength -= bytes;
						}
						else {
							Notify(HTTP_EVENT_GOT_DATA, p, mLength);
							mLength = 0;
						}

						if (mLength == 0)
							quit = true;

						break;
					}
				}
			}

			closesocket(mSocket);
			mSocket = INVALID_SOCKET;

			if (err)
				break;

			// 重定向
			if (mStatus >= 300 && mStatus < 400) {
				char* url = GetResponseHdrVal("location", 1);
				if (url) {
					if (mRedirectUrl)
						NBK_free(mRedirectUrl);
					mRedirectUrl = url_parse(mUrl, url);
					times++;
					Notify(HTTP_EVENT_REDIRECT, N_NULL, 0);

					// 请求重定向地址
					mGetMode = true;
					ReplaceReferer(mUrl);
					SetHeader(HTTP_KEY_REFERER, mReferer);
					nbk_strcpy(mUrl, mRedirectUrl);
					mLength = -1;

					// 清除地址信息
					freeaddrinfo(mAddrinfo); mAddrinfo = NULL;
					NBK_free(mHost); mHost = NULL;
				}
				else {
					stop = true;
					err = 31;
				}
			}
			else {
				stop = true;
				Notify(HTTP_EVENT_COMPLETE, N_NULL, 0);
			}
		}
		else {
			// 创建socket出错
			stop = N_TRUE;
			err = 1;
		}
	}

	if (err) {
		if (mSocket != INVALID_SOCKET) {
			closesocket(mSocket);
			mSocket = INVALID_SOCKET;
		}
		Notify((received > 0) ? HTTP_EVENT_COMPLETE : HTTP_EVENT_ERROR, N_NULL, 0);
	}

	Notify(HTTP_EVENT_CONNECT_CLOSED, N_NULL, 0);

	NBK_free(buf);
}

bool NHttp::CreateSocket()
{
	int ret;

	if (mAddrinfo == NULL) {
		uint16 port;
		mHost = url_parseHostPort(mUrl, &port);
		char portStr[8];
		sprintf(portStr, "%d", port);

		struct addrinfo hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		ret = getaddrinfo(mHost, portStr, &hints, &mAddrinfo);
		if (ret != 0)
			return false;
	}

	mSocket = socket(mAddrinfo->ai_family, mAddrinfo->ai_socktype, mAddrinfo->ai_protocol);
	if (mSocket == INVALID_SOCKET)
		return false;

	ret = connect(mSocket, mAddrinfo->ai_addr, mAddrinfo->ai_addrlen);
	if (ret == SOCKET_ERROR)
		return false;

	return true;
}

char* NHttp::CreateHeaders()
{
	char* blank = "";
	int size = 32;
	int i, pos, len;
	char* hdr;
	char clen[16];
	char* path;

	if (mLength > 0) {
		sprintf(clen, "%d", mLength);
		mHeaders[HTTP_KEY_CONTENT_LENGTH] = clen;
	}

	// GET url HTTP 1.1\r\n
	size += nbk_strlen(mUrl) + 2;

	// Host: url\r\n
	size += nbk_strlen(mHost) + 8;

	for (i = HTTP_KEY_BEGIN + 1; i < HTTP_KEY_LAST; i++) {
		if (mHeaders[i]) {
			// key: value\r\n
			size += nbk_strlen(HTTP_HEADER_KEY[i]) + nbk_strlen(mHeaders[i]) + 4;
		}
	}

	hdr = (char*)NBK_malloc(size);

	path = GetPath();
	if (path == N_NULL)
		path = blank;
	else if (*path == '/')
		path++;

	if (mGetMode)
		pos = sprintf(hdr, "GET /%s HTTP/1.1\r\n", path);
	else
		pos = sprintf(hdr, "POST /%s HTTP/1.1\r\n", path);

	len = sprintf(hdr + pos, "Host: %s\r\n", mHost);
	pos += len;

	for (i = HTTP_KEY_BEGIN + 1; i < HTTP_KEY_LAST; i++) {
		if (mHeaders[i]) {
			len = sprintf(hdr + pos, "%s: %s\r\n", HTTP_HEADER_KEY[i], mHeaders[i]);
			pos += len;
		}
	}

	nbk_strcpy(hdr + pos, "\r\n");
	pos += 2;

	mHeaders[HTTP_KEY_CONTENT_LENGTH] = N_NULL;
	mHeaders[HTTP_KEY_CONTENT_TYPE] = N_NULL;

	return hdr;
}

char* NHttp::GetPath()
{
	int pos = nbk_strfind(mUrl, "://");
	if (pos > 0) {
		char* p = mUrl + pos + 3;
		while (*p && *p != '/' && *p != '?')
			p++;
		if (*p)
			return p;
	}
	return N_NULL;
}

void NHttp::SetHeader(int key, char* value)
{
	if (key > HTTP_KEY_BEGIN && key < HTTP_KEY_LAST)
		mHeaders[key] = value;
}

void NHttp::ParseResponse()
{
	char** pairs = N_NULL;
	char *p, *q;
	int i = 0;

	// 解析回应码
	p = mResponseStr;
	while (*p != ' ')
		p++;
	p++;
	mStatus = NBK_atoi(p);

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

	for (i=0; pairs[i]; i+=2) {
		str_toLower(pairs[i], -1);
	}

	mResponse = pairs;

#ifdef DEBUG
	printf("RESPONSE: %s\n", mUrl);
	for (i = 0; mResponse[i]; i += 2) {
		printf("%s [ %s ]\n", mResponse[i], mResponse[i + 1]);
	}
#endif
}

void NHttp::Notify(int evt, char* data, int length)
{
	if (mListener)
		mListener->OnHttpEvent(evt, data, length);
}

char* NHttp::GetResponseHdrVal(const char* hdr, int index)
{
	int i, idx;

	if (mResponse) {
		for (i = idx = 0; mResponse[i]; i += 2) {
			if (nbk_strcmp(mResponse[i], hdr) == 0) {
				idx++;
				if (idx == index)
					return mResponse[i + 1];
			}
		}
	}

	return N_NULL;
}

void NHttp::Get(const char* url)
{
	nbk_strcpy(mUrl, url);
	mGetMode = true;
	mChunked = false;
	mGzip = false;
	mLength = -1;
	Start();
}

void NHttp::ReplaceReferer(const char* url)
{
	int len = nbk_strlen(url);
	if (mReferer)
		NBK_free(mReferer);
	mReferer = (char*)NBK_malloc(len + 1);
	nbk_strcpy(mReferer, url);
}
