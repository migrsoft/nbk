/*
 ============================================================================
 Name		: SimRequest.cpp
 Author	  : Yulun Wu
 Version	 : 1.0
 Copyright   : Baidu MIC
 Description : CSimRequest implementation
 ============================================================================
 */

#include "SimRequest.h"
#include "ResourceManager.h"
#include <coemain.h>
#include <bautils.h>
#include "../../../stdc/tools/str.h"

#define BUFFER_SIZE 32768

CSimRequest::CSimRequest(TInt aType) : CActive(EPriorityStandard)
{
    iType = aType;
    iCompressed = EFalse;
    iFileOpen = EFalse;
    iMime = NEMIME_UNKNOWN;
    
    iBuf = NULL;
}

CSimRequest* CSimRequest::NewLC(TInt aType)
{
    CSimRequest* self = new (ELeave) CSimRequest(aType);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CSimRequest* CSimRequest::NewL(TInt aType)
{
    CSimRequest* self = CSimRequest::NewLC(aType);
    CleanupStack::Pop(); // self;
    return self;
}

void CSimRequest::ConstructL()
{
    User::LeaveIfError(iTimer.CreateLocal());
    CActiveScheduler::Add(this);
}

CSimRequest::~CSimRequest()
{
    Cancel();
    iTimer.Close();
    
    if (iFileOpen) {
        iFile.Close();
    }
    
    if (iBuffer) {
        delete iBuffer;
        iBuffer = NULL;
    }
    
    if (iUrl) {
        delete iUrl;
        iUrl = NULL;
    }
    
    if (iBuf)
        NBK_free(iBuf);
}

void CSimRequest::DoCancel()
{
    iTimer.Cancel();
}

void CSimRequest::StartL()
{
    Cancel();
    iStage = EUninitialized;
    iTimer.After(iStatus, 5);
    SetActive();
}

void CSimRequest::RC_Cancel()
{
    DoCancel();
}

void CSimRequest::RunL()
{
    TBool stop = EFalse;
    
    if (iStage == EUninitialized) {
        iStage = EInitialized;
        
        if (iType == EHistoryCache) {
            stop = CheckFile();
        }
        else if (iType == EFileCache) {
            iObserver->OnSimRequestData(MSimRequestObserver::ESimReqHeader);
        }
        else if (iType == ESimNoCacheError) {
            iObserver->OnSimRequestData(MSimRequestObserver::ESimReqNoCache);
            stop = ETrue;
        }
        else if (iType == ESimApError) {
            iObserver->OnSimRequestData(MSimRequestObserver::ESimReqError);
            stop = ETrue;
        }
        else
            stop = ETrue;
    }
    else if (iStage != EError) {
        
        if (iType == EHistoryCache)
            stop = ReadData();
        else if (iType == EFileCache)
            stop = ReadCacheData();
        else
            stop = ETrue;
    }
    
    if (stop) {
        iStage = EError;
    }
    else {
        iTimer.After(iStatus, 5);
        SetActive();
    }
}

TInt CSimRequest::RunError(TInt aError)
{
    return aError;
}

// 检测临时缓存目录中是否存在请求的资源
TBool CSimRequest::CheckFile()
{
    CResourceManager* manager = CResourceManager::GetPtr();
    RFs& rfs = CCoeEnv::Static()->FsSession();
    
    iByteRead = 0;
    if (iFileOpen) {
        iFile.Close();
        iFileOpen = EFalse;
    }
    
    int len = manager->GetCachePath().Length();
    HBufC* name = HBufC::New(len + 64);
    name->Des().Zero();
    
    switch (iRequest->type) {
    // 主文档请求，包含主页面或云浏览页面数据包
    case NEREQT_MAINDOC:
    {
        TPtr p = name->Des();
        TPtr path = manager->GetCachePath();
        
        // 主文档URI数据文件
        p.Format(KCacheUriFormat, &path, iRequest->pageId);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                int size;
                iFile.Size(size);
                if (iUrl)
                    delete iUrl;
                iUrl = HBufC8::New(size);
                TPtr8 p = iUrl->Des();
                iFile.Read(p, size);
                iFile.Close();
            }
        }
        
        // gzip压缩的主文档
        p.Format(KCacheDocFormat, &path, iRequest->pageId);
        p.Append(KGzipSuffix);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                iFileOpen = ETrue;
                iCompressed = ETrue;
            }
            break;
        }

        // 未压缩的主文档
        p.Format(KCacheDocFormat, &path, iRequest->pageId);
        p.Append(KHtmlSuffix);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                iFileOpen = ETrue;
            }
            break;
        }
        
        // wap.wmlc
        p.Format(KCacheDocFormat, &path, iRequest->pageId);
        p.Append(KWbxmlSuffix);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                iFileOpen = ETrue;
                iMime = NEMIME_DOC_WMLC;
            }
            break;
        }
        break;
    }
    // 单张图片请求
    case NEREQT_IMAGE:
    {
        TPtrC8 url;
        TBuf8<32> md5str;
        TBuf<32> md5strU;
        url.Set((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
        CResourceManager::GetMsgDigestByMd5L(md5str, url);
        md5strU.Copy(md5str);
        
        // 图片以URI的MD5值命名的文件存储路径
        TPtr p = name->Des();
        TPtr path = manager->GetCachePath();
        p.Format(KCachePathFormat, &path, iRequest->pageId);
        p.Append(md5strU);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                iFileOpen = ETrue;
            }
        }
        break;
    }
    // CSS
    case NEREQT_CSS:
    {
        TPtrC8 url;
        TBuf8<32> md5str;
        TBuf<32> md5strU;
        url.Set((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
        CResourceManager::GetMsgDigestByMd5L(md5str, url);
        md5strU.Copy(md5str);
        
        TPtr p = name->Des();
        TPtr path = manager->GetCachePath();
        
        // 压缩
        p.Format(KCachePathFormat, &path, iRequest->pageId);
        p.Append(md5strU);
        p.Append(KCszSuffix);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                iFileOpen = ETrue;
                iMime = NEMIME_DOC_CSS;
                iCompressed = ETrue;
            }
        }

        // 非压缩
        p.Format(KCachePathFormat, &path, iRequest->pageId);
        p.Append(md5strU);
        p.Append(KCssSuffix);
        if (BaflUtils::FileExists(rfs, p)) {
            if (iFile.Open(rfs, p, EFileRead) == KErrNone) {
                iFileOpen = ETrue;
                iMime = NEMIME_DOC_CSS;
            }
        }
        break;
    }
    } // switch
    
    delete name;

    if (iFileOpen) {
        iFile.Size(iSize);
        if (iBuffer == NULL)
            iBuffer = HBufC8::New(BUFFER_SIZE);
        iObserver->OnSimRequestData(MSimRequestObserver::ESimReqHeader);
        return EFalse;
    }
    else {
        iObserver->OnSimRequestData(MSimRequestObserver::ESimReqError);
        return ETrue;
    }
}

TBool CSimRequest::ReadData()
{
    if (iFileOpen) {
        TPtr8 p = iBuffer->Des();
        if (iFile.Read(p, BUFFER_SIZE) == KErrNone) {
            iObserver->OnSimRequestData(MSimRequestObserver::ESimReqData);
            iByteRead += p.Length();
            if (iByteRead >= iSize) {
                iFile.Close();
                iFileOpen = EFalse;
                iObserver->OnSimRequestData(MSimRequestObserver::ESimReqComplete);
            }
            else
                return EFalse;
        }
    }
    
    return ETrue;
}

TBool CSimRequest::ReadCacheData()
{
    if (iBuf == NULL) {
        iBuf = (char*)NBK_malloc(BUFFER_SIZE);
        iByteRead = 0;
    }
    
    iRead = cacheMgr_getData(iCacheMgr, iCacheId, iBuf, BUFFER_SIZE, iByteRead);
    iByteRead += iRead;
    iObserver->OnSimRequestData(MSimRequestObserver::ESimReqData);
    
    if (iByteRead >= iSize) {
        iObserver->OnSimRequestData(MSimRequestObserver::ESimReqComplete);
        return ETrue;
    }
    
    return EFalse;
}

TBool CSimRequest::GetNextDataPart(TPtrC8& aDataPart)
{
    if (iBuf)
        aDataPart.Set((TUint8*)iBuf, iRead);
    else
        aDataPart.Set(iBuffer->Des());
    
    return (iByteRead >= iSize) ? EFalse : ETrue;
}

void CSimRequest::ReleaseData()
{
    if (iBuffer)
        iBuffer->Des().Zero();
}
