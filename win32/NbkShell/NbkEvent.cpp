#include "stdafx.h"
#include "NbkEvent.h"
#include "../../stdc/inc/config.h"

int NbkEvent::_alloc = 0;
int NbkEvent::_free = 0;

void NbkEvent::MemoryInfo()
{
	TCHAR msg[64];
	wsprintf(msg, _T("*** EVENT alloc: %d free: %d ***\n"), _alloc, _free);
	OutputDebugString(msg);
}

NbkEvent::NbkEvent(int id)
{
	_alloc++;

	mId = id;
	mId2 = 0;

	mV1 = mV2 = mV3 = 0;

	mData = NULL;
	mDataLen = 0;

	mUrl = NULL;
	mDelUrl = false;

	mFunc = mUser = NULL;
}

NbkEvent::~NbkEvent()
{
	_free++;

	ReleaseUrl();

	if (mData) {
		NBK_free(mData);
		mData = NULL;
	}
}

void NbkEvent::SetCallback(int pageId, void* func, void* user)
{
	mId2 = pageId;
	mFunc = func;
	mUser = user;
}

void NbkEvent::GetCallback(int* pageId, void** func, void** user)
{
	*pageId = mId2;
	*func = mFunc;
	*user = mUser;
}

void NbkEvent::SetUrl(const char* url, bool del)
{
	ReleaseUrl();
	mUrl = (char*)url;
	mDelUrl = del;
}

void NbkEvent::ReleaseUrl()
{
	if (mUrl && mDelUrl) {
		NBK_free(mUrl);
		mUrl = NULL;
		mDelUrl = false;
	}
}

void NbkEvent::SetPageSize(int w, int h)
{
	mV1 = w;
	mV2 = h;
}
