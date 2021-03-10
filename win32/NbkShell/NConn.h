#pragma once

#include "../../stdc/inc/config.h"
#include "../../stdc/loader/loader.h"

class LoaderMgr;

class NConn {

protected:
	int			mConnId;
	NRequest*	mRequest;

	LoaderMgr*	mLoader;

public:
	NConn(int id, NRequest* req, LoaderMgr* loader);
	virtual ~NConn();

	int GetId() const { return mConnId; }

	virtual void Submit();
	virtual void Run() {}

	void ReceiveHeader(int mime, int encoding, int length);
	void ReceiveData(char* data, int length, int comprLen);
	void ReceiveComplete();
	void ReceiveError(int error);

	char* GetHistoryFileName();

private:
	static DWORD WINAPI Proc(LPVOID pParam);
};
