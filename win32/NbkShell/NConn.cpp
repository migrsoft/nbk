#include "stdafx.h"
#include "NConn.h"
#include "LoaderMgr.h"
#include "../../stdc/tools/str.h"

NConn::NConn(int id, NRequest* req, LoaderMgr* loader)
{
	mConnId = id;
	mRequest = req;
	mLoader = loader;

	smartptr_get(mRequest);
}

NConn::~NConn()
{
	smartptr_release(mRequest);
}

DWORD WINAPI NConn::Proc(LPVOID pParam)
{
	NConn* c = (NConn*)pParam;
	c->Run();
	delete c;
	return 0;
}

void NConn::Submit()
{
	CreateThread(NULL, 0, NConn::Proc, this, 0, NULL);
}

void NConn::ReceiveHeader(int mime, int encoding, int length)
{
	mLoader->ReceiveHeader(this, mime, encoding, length);
}

void NConn::ReceiveData(char* data, int length, int comprLen)
{
	mLoader->ReceiveData(this, data, length, comprLen);
}

void NConn::ReceiveComplete()
{
	mLoader->ReceiveComplete(this);
}

void NConn::ReceiveError(int error)
{
	mLoader->ReceiveError(this, error);
}

char* NConn::GetHistoryFileName()
{
	const int maxNameLen = 63;
	char* path = (char*)NBK_malloc(maxNameLen + 32);

	LoaderMgr::GetHistoryPath(mRequest->pageId, path);
	char* name = path + nbk_strlen(path);

	if (mRequest->type == NEREQT_MAINDOC) {
		nbk_strcpy(name, "main.htm");
	}
	else {
		bool useMd5 = false;
		int quest = str_lastIndexOf(mRequest->url, "?");
		int slash = str_lastIndexOf(mRequest->url, "/");
		int len = nbk_strlen(mRequest->url);

		if (quest != -1) {
			// url 包含查询参数，采用 MD5 做为文件名
			useMd5 = true;
		}
		else if (slash < 7 || slash == len - 1 || len - slash > maxNameLen) {
			// 文件名无效
			useMd5 = true;
		}

		if (useMd5) {
			char* m = str_md5(mRequest->url);
			nbk_strcpy(name, m);
			NBK_free(m);
		}
		else {
			nbk_strcpy(name, &mRequest->url[slash + 1]);
		}
	}

	return path;
}
