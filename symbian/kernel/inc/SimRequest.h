/*
 ============================================================================
 Name		: SimRequest.h
 Author	  : Yulun Wu
 Version	 : 1.0
 Copyright   : Baidu MIC
 Description : CSimRequest declaration
 ============================================================================
 */

#ifndef SIMREQUEST_H
#define SIMREQUEST_H

#include <e32base.h>
#include <e32std.h>
#include <f32file.h>
#include <http/mhttpdatasupplier.h>
#include "../../../stdc/inc/config.h"
#include "../../../stdc/loader/loader.h"
#include "../../../stdc/loader/cacheMgr.h"

class MSimRequestObserver
{
public:
    enum TStatus {
        ESimReqHeader,
        ESimReqData,
        ESimReqComplete,
        ESimReqError,
        ESimReqNoCache
    };
    
public:
    virtual void OnSimRequestData(MSimRequestObserver::TStatus aStatus) = 0;
};

class CSimRequest : public CActive, public MHTTPDataSupplier
{
public:
    enum TRequest {
        EHistoryCache = 1,
        EFileCache,
        ESimNoCacheError,
        ESimApError
    };
    
public:
    ~CSimRequest();
    static CSimRequest* NewL(TInt aType);
    static CSimRequest* NewLC(TInt aType);

public:
    void SetObserver(MSimRequestObserver* aObserver) { iObserver = aObserver; }
    void SetNbkRequest(NRequest* aRequest) { iRequest = aRequest; }
    void SetFsCacheIndex(TInt aIndex) { iCacheId = aIndex; }
    void StartL();
    
    void SetSize(int length) { iSize = length; }
    void SetMime(nid mime) { iMime = mime; }
    void SetCompressed(TBool b) { iCompressed = b; }
    
    void SetCacheMgr(NCacheMgr* mgr) { iCacheMgr = mgr; }
    
    HBufC8* GetUrl() { return iUrl; }
    
    int GetSize() const { return iSize; }
    TBool IsCompressed() const { return iCompressed; }
    nid Mime() const { return iMime; }
    
    void RC_Cancel();
    
public:
    // from MHTTPDataSupplier
    TBool GetNextDataPart(TPtrC8& aDataPart);
    void ReleaseData();
    TInt OverallDataSize() { return GetSize(); }
    TInt Reset() { return KErrNone; }

private:
    CSimRequest(TInt aType);
    void ConstructL();
    
    // 读取历史缓存
    TBool CheckFile();
    TBool ReadData();
    
    TBool ReadCacheData();

private:
    // From CActive
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);

private:
    enum TSimReqState {
        EUninitialized,
        EInitialized,
        EError
    };

private:
    TInt iType;
    
    TInt iStage;
    RTimer iTimer;
    
    TBool iCompressed;
    nid iMime;
    
    RFile iFile;
    TBool iFileOpen;
    HBufC8* iUrl;
    HBufC8* iBuffer;
    int iSize;
    int iByteRead;
    MSimRequestObserver* iObserver;
    NRequest* iRequest;
    TInt iCacheId;
    
    NCacheMgr* iCacheMgr;
    char* iBuf;
    int iRead;
};

#endif
