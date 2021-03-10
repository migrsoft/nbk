#pragma once

#ifdef __WIN32__
	#include "win32/jni.h"
#endif
#ifdef __ANDROID__
	#include <jni.h>
#endif
#include "../../stdc/loader/loader.h"

class LoaderMgr {

public:
	JNIEnv*		JNI_env;
	jweak		OBJ_LoaderMgr;
	jmethodID	MID_loadFile;
	jmethodID	MID_loadHistory;
	jmethodID	MID_httpGet;
	jmethodID	MID_deleteHistoryDir;

private:

	typedef struct _Conn {
		int				id;
		NRequest*		req;
	} Conn;

	static const int MAX_REQUEST = 10;

	int mCounter;
	Conn mConns[MAX_REQUEST];

public:
	LoaderMgr();
	~LoaderMgr();

	nbool Load(NRequest* request);

	NRequest* FindRequest(nid pageId, int connId, const char* url);
	void RemoveRequest(NRequest* request);
	void RemoveAllRequests(nid pageId);

	void ResourceClean(nid pageId);

private:
	int AddRequest(NRequest* request);
};
