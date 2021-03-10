#include "stdafx.h"
#include "NConnCache.h"
#include "LoaderMgr.h"
#include "../../stdc/loader/ngzip.h"

NConnCache::NConnCache(int cacheId, int id, NRequest* req, LoaderMgr* loader)
	: NConn(id, req, loader)
{
	mCacheId = cacheId;
	mMime = 0;
	mEncoding = 0;
	mLength = 0;
	mGzipped = false;
}

NConnCache::~NConnCache()
{
	TCHAR msg[64];
	wsprintf(msg, _T("NConnCache (%d) finish.\n"), GetId());
	OutputDebugString(msg);
}

void NConnCache::SetInfo(int mime, int encoding, int length, bool gzip)
{
	mMime = mime;
	mEncoding = encoding;
	mLength = length;
	mGzipped = gzip;
}

void NConnCache::Run()
{
	int read = 0;
	char* buf;
	int len;
	NGzip* gzip = NULL;

	ReceiveHeader(mMime, mEncoding, mLength);

	if (mGzipped)
		gzip = ngzip_create();

	buf = (char*)NBK_malloc(BUF_SIZE);
	while (read < mLength) {
		len = mLoader->LoadCacheData(mCacheId, buf, BUF_SIZE, read);
		if (len > 0) {
			char* data = buf;
			int uncomprLen = len;

			if (gzip) {
				uint8* unzip;
				ngzip_addZippedData(gzip, (uint8*)data, len);
				unzip = ngzip_getData(gzip, &uncomprLen);
				data = (char*)unzip;
			}

			if (data) {
				char* d = (char*)NBK_malloc(uncomprLen);
				NBK_memcpy(d, data, uncomprLen);
				ReceiveData(d, uncomprLen, len);
			}

			read += len;
		}
		else
			break;
	}

	if (read == mLength)
		ReceiveComplete();
	else
		ReceiveError(-1);

	if (gzip)
		ngzip_delete(&gzip);

	NBK_free(buf);
}
