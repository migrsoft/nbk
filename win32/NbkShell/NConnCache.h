#pragma once

#include "NConn.h"

class LoaderMgr;

class NConnCache : public NConn {

private:
	static const int BUF_SIZE = 1024 * 8;

	int		mCacheId;
	int		mMime;
	int		mEncoding;
	int		mLength;
	bool	mGzipped;

public:
	NConnCache(int cacheId, int id, NRequest* req, LoaderMgr* loader);
	virtual ~NConnCache();

	void SetInfo(int mime, int encoding, int length, bool gzip);

	virtual void Run();
};
