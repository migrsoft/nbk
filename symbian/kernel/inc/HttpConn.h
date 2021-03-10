/*
 ============================================================================
 Name		: HttpConn.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CHttpConn declaration
 ============================================================================
 */

#ifndef HTTPCONN_H
#define HTTPCONN_H

#include "../../stdc/loader/ngzip.h"
#include <e32std.h>
#include <e32base.h>
#include <http/mhttptransactioncallback.h>
#include <http/mhttpdatasupplier.h>
#include "MResConn.h"
#include "SimRequest.h"

// CLASS DECLARATION

class CResourceManager;
class CBufFileWriter;

/**
 *  CHttpConn
 * 
 */
class CHttpConn : public CBase, public MResConn,
                  public MHTTPTransactionCallback,
                  public MHTTPDataSupplier,
                  public MSimRequestObserver
{
public:
    ~CHttpConn();
    static CHttpConn* NewL(NRequest* request);
    static CHttpConn* NewLC(NRequest* request);

public:
    // from MConnection
    void RC_Submit();
    void RC_Cancel();
    
public:
    // from MHTTPTransactionCallback
    void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
    TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent);
    
public:
    // from MHTTPDataSupplier
    TBool GetNextDataPart(TPtrC8& aDataPart);
    void ReleaseData();
    TInt OverallDataSize();
    TInt Reset();

public:
    // CSimRequestObserver
    void OnSimRequestData(MSimRequestObserver::TStatus aStatus);
    
public:
    void SetManager(CResourceManager* aManager) { iManager = aManager; }
    void OnIapFailed(void);
    
private:
    CHttpConn(NRequest* request);
    void ConstructL();
    
    void SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue);
    void AddHeaders();
    void Complete(int error);
    int HandleRedirectEvent(int event, int aHttpStatus);
    TBool IsNonHttpRedirect();

    void CheckGzipL(const RHTTPTransaction& aTrans);
    
    void AddCookiesL(RHTTPTransaction& aTransaction);
    void CheckCookiesL(RHTTPTransaction& aTransaction);
    
    // 创建缓存文件
    void CreateCacheFile(int aMime);
    // 缓存数据
    void Cache(TPtrC8& aData);
    // 关闭缓存文件
    void CacheClose();

    // 组装上传文件数据缓存，返回数据长度
    int GenerateUploadData(TBool aStandard);
    
    void CreateNgzip();

private:
    RHTTPTransaction* m_transaction;
    bool m_isDone;
    
    HBufC8* m_frag; // owned
    HBufC8* m_urlResponse; // owned
    HBufC8* m_contentType; // owned
    HBufC8* m_encoding; // owned
    HBufC* iCacheFile;
    CBufFileWriter* iBufFileWriter;
    
    int m_maxSize;
    int iComprLen;
    int iPostPos;
    int iSent;
    int iBodyLen; // post 数据体长度（不含上传文件部分）
    int iBodyFileLen; // post 数据体文件部分长度
    
    CResourceManager* iManager;
    
    // 数据压缩/解压
    NBK_gzip* iNgz;
    uint8* iDeflatedBody;
    
    CSimRequest* iSimRequest;
    
    bool iRedirect;
    int iCacheId; // FsCache 缓存文件索引值

    RFile* iPostFile; // 文件上传缓存文件
    HBufC8* iPostFileBuf;
    int iPageId;
    
    int iSubmitTimes;
};

#endif // HTTPCONN_H
