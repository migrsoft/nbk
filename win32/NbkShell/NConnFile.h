#pragma once

#include "NConn.h"

class NConnFile : public NConn {

private:
	static const int BUF_SIZE = 1024 * 8;

public:
	NConnFile(int id, NRequest* req, LoaderMgr* loader);
	virtual ~NConnFile();

	virtual void Run();

protected:
	virtual char* GetFilePath();
};
