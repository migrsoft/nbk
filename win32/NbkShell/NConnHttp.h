#pragma once

#include "NConn.h"
#include "NHttp.h"
#include "../../stdc/loader/ngzip.h"
#include "../../stdc/tools/strBuf.h"

class NConnHttp : public NConn, public NHttpListener {

private:
	NHttp*	mHttp;
	NGzip*	mGzip;
	
	char*	mCookie;

	int		mCacheId;

	bool	mBeRedirect;

	bool	mPicPage; // 请求页面返回图片
	int		mPicPageLength;

public:
	NConnHttp(int id, NRequest* req, LoaderMgr* loader);
	~NConnHttp();

	virtual void Run();

	virtual void OnHttpEvent(int evt, char* data, int len);

private:
	void ReplaceCookie(char* cookie);
	void CollectCookie();
	char* GetCookie(char* url);

private:
	char*		mWritePath;
	NStrBuf*	mWriteBuffer; // 存储数据用于历史操作

	void SaveHistoryResource();
};
