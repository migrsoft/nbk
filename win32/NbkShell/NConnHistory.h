#pragma once

#include "NConnFile.h"

class NConnHistory : public NConnFile {

public:
	NConnHistory(int id, NRequest* req, LoaderMgr* loader);
	virtual ~NConnHistory();

protected:
	virtual char* GetFilePath();
};
