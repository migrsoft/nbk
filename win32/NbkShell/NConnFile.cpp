#include "stdafx.h"
#include "NConnFile.h"
#include "LoaderMgr.h"
#include "../../stdc/tools/str.h"

NConnFile::NConnFile(int id, NRequest* req, LoaderMgr* loader) : NConn(id, req, loader)
{
}

NConnFile::~NConnFile()
{
	TCHAR msg[64];
	wsprintf(msg, _T("NConnFile (%d) finish.\n"), GetId());
	OutputDebugString(msg);
}

void NConnFile::Run()
{
	char* path = GetFilePath();

	FILE* f = fopen(path, "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		int length = ftell(f);
		fseek(f, 0, SEEK_SET);

		ReceiveHeader(0, 0, length);

		int size;
		char* buf;
		while (length > 0) {
			size = (length > BUF_SIZE) ? BUF_SIZE : length;
			buf = (char*)NBK_malloc(size);
			fread(buf, 1, size, f);
			length -= size;
			ReceiveData(buf, size, size);
		}

		fclose(f);
		ReceiveComplete();
	}
	else { // 打开文件出错
		ReceiveError(404);
	}
	NBK_free(path);
}

char* NConnFile::GetFilePath()
{
	// 转换 file:///c:/path/some.htm => c:\\path\\some.htm

	char* url = (char*)NBK_malloc(nbk_strlen(mRequest->url));

	if (mRequest->url[9] == ':')
		nbk_strcpy(url, mRequest->url + 8);
	else
		nbk_strcpy(url, mRequest->url + 7);

	int len = nbk_strlen(url);
	for (int i=0; i < len; i++)
		if (url[i] == '/')
			url[i] = '\\';

	return url;
}
