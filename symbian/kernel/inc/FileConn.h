/*
 ============================================================================
 Name		: FileConn.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CFileConn declaration
 ============================================================================
 */

#ifndef FILECONN_H
#define FILECONN_H

#include <e32base.h>	// For CActive, link against: euser.lib
#include <e32std.h>		// For RTimer, link against: euser.lib
#include <f32file.h>
#include "MResConn.h"
#include "nbk_limit.h"

class CResourceManager;

class CFileConn: public CActive, public MResConn
{
public:
    ~CFileConn();
    static CFileConn* NewL(NRequest* request);
    static CFileConn* NewLC(NRequest* request);
    
    void SetManager(CResourceManager* aManager) { iManager = aManager; }
    
public:
    // from MConnection
    void RC_Submit();
    void RC_Cancel();

public:
    void StartL(TTimeIntervalMicroSeconds32 aDelay);

private:
    CFileConn(NRequest* request);
    void ConstructL();

private:
    // from CActive
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);
    
private:
    void ParsePath();
    
    void CreateCacheFile(int aMime);
    void Cache(TPtr8& aData);
    void CacheClose();

private:
    enum TFileConnState
    {
        EUninitialized, // Uninitialized
        EInitialized, // Initalized
        EError
    // Error condition
    };

private:
    TInt    iState; // State of the active object
    RTimer  iTimer; // Provides async timing service
    
    TBuf<MAX_FILE_PATH>   iFilePath;
    RFile   iFile;
    HBufC8* iBuffer;
    int     iTotalSize;
    int     iByteRead;
    
    CResourceManager*   iManager;
    HBufC*              iCacheFile;
};

#endif // FILECONN_H
