/*
 ============================================================================
 Name		: FileConn.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CFileConn implementation
 ============================================================================
 */

#include "../../../stdc/loader/loader.h"
#include "../../../stdc/tools/str.h"
#include <coemain.h>
#include "FileConn.h"
#include "ResourceManager.h"
#include "Probe.h"

#define READ_BUFFER_SIZE    16384

CFileConn::CFileConn(NRequest* request)
    : CActive(EPriorityStandard)
    , MResConn(request)
    , iBuffer(NULL)
{
}

CFileConn* CFileConn::NewLC(NRequest* request)
{
    CFileConn* self = new (ELeave) CFileConn(request);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CFileConn* CFileConn::NewL(NRequest* request)
{
    CFileConn* self = CFileConn::NewLC(request);
    CleanupStack::Pop(); // self;
    return self;
}

void CFileConn::ConstructL()
{
    iConnType = EFileConn;
    User::LeaveIfError(iTimer.CreateLocal()); // Initialize timer
    CActiveScheduler::Add(this); // Add to scheduler
}

CFileConn::~CFileConn()
{
    Cancel(); // Cancel any request, if outstanding
    iTimer.Close(); // Destroy the RTimer object
    if (iBuffer) {
        delete iBuffer;
        iBuffer = NULL;
    }
    iFile.Close();
    
    if (iCacheFile) {
        delete iCacheFile;
        iCacheFile = NULL;
    }
}

void CFileConn::DoCancel()
{
    iTimer.Cancel();
}

void CFileConn::StartL(TTimeIntervalMicroSeconds32 aDelay)
{
    Cancel(); // Cancel any request, just to be sure
    iState = EUninitialized;
    iTimer.After(iStatus, aDelay); // Set for later
    SetActive(); // Tell scheduler a request is active
}

void CFileConn::RunL()
{
    TBool run = ETrue;
    
    if (iState == EUninitialized) {
        // Do something the first time RunL() is called
        iState = EInitialized;
        
        // 分析文件路径
        ParsePath();
        
        // 打开文件
        if (iFile.Open(CCoeEnv::Static()->FsSession(), iFilePath, EFileRead) == KErrNone) {
            iBuffer = HBufC8::NewL(READ_BUFFER_SIZE);
            iByteRead = 0;
            iFile.Size(iTotalSize);
            
            NRespHeader header;
            NBK_memset(&header, 0, sizeof(NRespHeader));
            header.content_length = iTotalSize;
            
            if (iFilePath.Find(KGifSuffix) != KErrNotFound)
                header.mime = NEMIME_IMAGE_GIF;
            else if (iFilePath.Find(KJpegSuffix) != KErrNotFound)
                header.mime = NEMIME_IMAGE_JPEG;
            else if (iFilePath.Find(KHtmlSuffix) != KErrNotFound)
                header.mime = NEMIME_DOC_HTML;
            else if (iFilePath.Find(KCssSuffix) != KErrNotFound)
                header.mime = NEMIME_DOC_CSS;
            else
                header.mime = NEMIME_UNKNOWN;
            
            CResourceManager::OnReceiveHeader(this, &header);
            
            CreateCacheFile(header.mime);
        }
        else {
            CResourceManager::OnError(this, 404);
            run = EFalse;
        }
    }
    else if (iState != EError) {
        TPtr8 dataPtr(iBuffer->Des());
        if (iFile.Read(dataPtr, READ_BUFFER_SIZE) == KErrNone) {
            
            CResourceManager::OnReceiveData(this, (char*)dataPtr.Ptr(), iBuffer->Length(), iBuffer->Length());
            Cache(dataPtr);
            
            iByteRead += iBuffer->Length();
            if (iByteRead >= iTotalSize) {
                // 读取完成
                CResourceManager::OnComplete(this);
                CacheClose();
                iConnState = EFinished;
                run = EFalse;
            }
        }
        else {
            
        }
    }
    
    if (run) {
        iTimer.After(iStatus, 10);
        SetActive(); // Tell scheduler a request is active
    }
}

TInt CFileConn::RunError(TInt aError)
{
    return aError;
}

void CFileConn::RC_Submit()
{
    iConnState = EActive;
    StartL(0);
}

void CFileConn::RC_Cancel()
{
    DoCancel();
    iConnState = EFinished;
}

void CFileConn::ParsePath()
{
    // 文件地址 file://c/data/nbk_sample.htm
    // 转换为 c:\data\nbk_sample.htm
    
    char path[MAX_FILE_PATH];
    char *p = iRequest->url + 7;
    int i = 0;
    while (i < MAX_FILE_PATH && *p) {
        
        if (i == 0) {
            path[i++] = *p;
            path[i] = ':';
        }
        else {
            path[i] = (*p == '/') ? '\\' : *p;
        }
        p++;
        i++;
    }
    i = (i == MAX_FILE_PATH) ? i - 1 : i;
    path[i] = 0;
    
    TPtr8 pathPtr((TUint8*)path, i, i);
    iFilePath.Copy(pathPtr);
}

void CFileConn::CreateCacheFile(int aMime)
{
    if (iCacheFile == NULL) {
        int len = iManager->GetCachePath().Length();
        iCacheFile = HBufC::New(len + 64);
    }
    iCacheFile->Des().Zero();
    
    if (MainDoc()) {
        iManager->CleanCachedPage(iRequest->pageId);
    }
    
    if (MainDoc() && iRequest->url) {
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        TPtrC8 url((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
        p.Format(KCacheUriFormat, &path, iRequest->pageId);
        CProbe::FileInit(p, ETrue);
        CProbe::FileWrite(p, url);
    }

    if (aMime == NEMIME_DOC_HTML ||
        aMime == NEMIME_DOC_XHTML ||
        aMime == NEMIME_DOC_WML) {
        
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        p.Format(KCacheDocFormat, &path, iRequest->pageId);
        p.Append(KHtmlSuffix);
        
        CProbe::FileInit(p, ETrue);
    }
}

void CFileConn::Cache(TPtr8& aData)
{
    if (iCacheFile && iCacheFile->Length()) {
        CProbe::FileWrite(iCacheFile->Des(), aData);
    }
}

void CFileConn::CacheClose()
{
    if (iCacheFile) {
        iCacheFile->Des().Zero();
    }
}
