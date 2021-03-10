#pragma once

#include "../../stdc/loader/loader.h"
#include "../../stdc/loader/cookiemgr.h"
#include "../../stdc/loader/cachemgr.h"

class NbkCore;
class NConn;
class NbkEvent;

class LoaderMgr {

private:
	NbkCore*	mCore;

	typedef struct _Conn {
		int			id;
		NRequest*	req;
	} Conn;

	static const int MAX_REQUEST = 10;

	int mCounter;
	Conn mConns[MAX_REQUEST];

	CRITICAL_SECTION	mCookieAccessCS;
	NCookieMgr*			mCookieMgr;

	CRITICAL_SECTION	mCacheAccessCS;
	NCacheMgr*			mCacheMgr;

public:
	LoaderMgr(NbkCore* core);
	~LoaderMgr();

	void Init();
	void Quit();

	nbool Load(NRequest* request);

	void RemoveAllRequests(nid pageId);

	static bool IsPathExist(const char* path);
	static void MakeAllDir(const char* path);
	static void RemoveDir(const char* path);
	static void RemoveMultiDir(const char* path, const char* match);

	static void GetHistoryPath(int pageId, char* path);

public:
	void ReceiveHeader(NConn* conn, int mime, int encoding, int length);
	void ReceiveData(NConn* conn, char* data, int length, int comprLen);
	void ReceiveComplete(NConn* conn);
	void ReceiveError(NConn* conn, int error);

	void HandleEvent(NbkEvent* evt);

	char* GetCookie(const char* url);
	void SetCookie(const char* host, char* cookie);

	int StoreCache(const char* url, int mime, int encoding, bool gzip);
	void SaveCacheData(int cacheId, char* data, int length, bool over);

	int FindCache(const char* url, int* mime, int* encoding, int* length, bool* gzip);
	int LoadCacheData(int cacheId, char* buf, int size, int from);

private:
	int AddRequest(NRequest* request);
	void RemoveRequest(NRequest* request);
	NRequest* FindRequest(int connId);
};
