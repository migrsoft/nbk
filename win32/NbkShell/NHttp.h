#pragma once

class NHttpListener {
public:
	virtual void OnHttpEvent(int evt, char* data, int len) = 0;
};

class NHttp {

public:
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

private:
	static const int BUF_SIZE = 1024 * 8;

	struct addrinfo*	mAddrinfo;
	SOCKET				mSocket;

	char*	mUrl;
	char*	mReferer;
	char*	mRedirectUrl;
	char*	mHost;
	int		mStatus;
	int		mLength;

	char*	mHeaders[HTTP_KEY_LAST];

	bool	mGetMode;
	bool	mChunked;
	bool	mGzip;

	char*	mResponseStr;
	char**	mResponse;

	NHttpListener*	mListener;

public:
	NHttp();
	~NHttp();

	void SetListener(NHttpListener* listener) { mListener = listener; }

	void Get(const char* url);

	bool IsGzipped() { return mGzip; }
	int GetLength() { return mLength; }
	char* GetHeader(int key);
	void SetHeader(int key, char* value);

	char* GetRedirectUrl() { return mRedirectUrl; }
	char* GetHost() { return mHost; }

	char* GetResponseHdrVal(const char* hdr, int index);

private:
	void Start();
	bool CreateSocket();
	char* CreateHeaders();
	char* GetPath();
	void ParseResponse();
	void Notify(int evt, char* data, int length);
	void ReplaceReferer(const char* url);
};
