#include "stdafx.h"
#include "NConnHistory.h"

NConnHistory::NConnHistory(int id, NRequest* req, LoaderMgr* loader) : NConnFile(id, req, loader)
{
}

NConnHistory::~NConnHistory()
{
	TCHAR msg[64];
	wsprintf(msg, _T("NConnHistory (%d) finish.\n"), GetId());
	OutputDebugString(msg);
}

char* NConnHistory::GetFilePath()
{
	return GetHistoryFileName();
}
