/*
 ============================================================================
 Name		: HttpConn.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CHttpConn implementation
 ============================================================================
 */

#include "../../../stdc/inc/config.h"
#include "../../../stdc/loader/loader.h"
#include "../../../stdc/loader/cachemgr.h"
#include "../../../stdc/loader/cookiemgr.h"
#include "../../../stdc/loader/upcmd.h"
#include "../../../stdc/loader/url.h"
#include "../../../stdc/tools/str.h"
#include "../../../stdc/tools/unicode.h"
#include "../../../stdc/render/imagePlayer.h"
#include <coemain.h>
#include <httpstringconstants.h>
#include <rhttpheaders.h>
#include <mhttpdatasupplier.h>
#include <thttpevent.h>
#include <string.h>
#include "HttpConn.h"
#include "ResourceManager.h"
#include "NBKPlatformData.h"
#include "Probe.h"
#include "NetAddr.h"
#include "BufFileWriter.h"
#include "NBrKernel.h"

#define DEBUG_NET   0
#define DEBUG_POST  0
#define DEBUG_CACHE 0

//#define TIME_LOG

#define NBK_POST_DATA_CMP   1

_LIT8(KUserAgent,           "Mozilla/5.0 (SymbianOS/9.2; Series60/3.2; Nokia; NBK)");
_LIT8(KAccept,              "*/*");
_LIT8(KAcceptCharset,       "utf-8, iso-8859-1;");
_LIT8(KKeepAlive,           "keep-alive");
_LIT8(KNoCache,             "no-cache");
_LIT8(KGzipDeflateStr,      "gzip, deflate");
_LIT8(KGzipStr,             "gzip");
_LIT8(KFieldSeparator,      ": ");
_LIT8(KTypeOctet,           "application/octet-stream");
_LIT8(KTypeUrlencoded,      "application/x-www-form-urlencoded");

_LIT(KPostFileStreamFmt,    "c:\\data\\nbkpost%05d");

//const int KSecureHttpSchemeLength = 8;
const int KMaxCookieLineLength = 4096;
const int KMaxPostBufSize = 1024;

CHttpConn::CHttpConn(NRequest* request)
    : MResConn(request)
{
    m_isDone = false;
    m_transaction = NULL;
    m_frag = NULL;
    m_urlResponse = NULL;
    m_contentType = NULL;
    m_encoding = NULL;
    m_maxSize = 0;
}

CHttpConn::~CHttpConn()
{
    if (m_transaction) {
        m_transaction->Cancel();
        m_transaction->Close();
        delete m_transaction;
        m_transaction = NULL;
    }

    if (m_frag) {
        delete m_frag;
        m_frag = NULL;
    }

    if (m_urlResponse) {
        delete m_urlResponse;
        m_urlResponse = NULL;
    }

    if (m_contentType) {
        delete m_contentType;
        m_contentType = NULL;
    }

    if (m_encoding) {
        delete m_encoding;
        m_encoding = NULL;
    }
    
    if (iNgz) {
        ngzip_closeUnzip(iNgz);
        ngzip_delete(&iNgz);
    }
    if (iDeflatedBody) {
        NBK_free(iDeflatedBody);
        iDeflatedBody = N_NULL;
    }
    
    if (iCacheFile) {
        delete iCacheFile;
        iCacheFile = NULL;
    }
    
    if (iSimRequest) {
        delete iSimRequest;
        iSimRequest = NULL;
    }
    
    if (iPostFile) {
        iPostFile->Close();
        delete iPostFile;
        iPostFile = NULL;

        RFs& rfs = CCoeEnv::Static()->FsSession();
        TBuf<64> postfn;
        postfn.Format(KPostFileStreamFmt, iPageId);
        rfs.Delete(postfn);
    }
    if (iPostFileBuf) {
        delete iPostFileBuf;
        iPostFileBuf = NULL;
    }
    
    if (iBufFileWriter) {
        delete iBufFileWriter;
        iBufFileWriter = NULL;
    }
}

CHttpConn* CHttpConn::NewLC(NRequest* request)
{
    CHttpConn* self = new (ELeave) CHttpConn(request);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CHttpConn* CHttpConn::NewL(NRequest* request)
{
    CHttpConn* self = CHttpConn::NewLC(request);
    CleanupStack::Pop(); // self;
    return self;
}

void CHttpConn::ConstructL()
{
    iConnType = EHttpConn;
}

void CHttpConn::SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue)
{
    RHTTPSession& session = iManager->httpSession();
    RStringF valStr = session.StringPool().OpenFStringL(aHdrValue);
    CleanupClosePushL(valStr);
    THTTPHdrVal val(valStr);
    aHeaders.SetFieldL(session.StringPool().StringF(aHdrField, RHTTPSession::GetTable()), val);
    CleanupStack::PopAndDestroy();
}

void CHttpConn::AddHeaders()
{
    RHTTPHeaders hdr = m_transaction->Request().GetHeaderCollection();
    SetHeaderL(hdr, HTTP::EUserAgent, KUserAgent);
    SetHeaderL(hdr, HTTP::EAccept, KAccept);
    SetHeaderL(hdr, HTTP::EAcceptCharset, KAcceptCharset);
    SetHeaderL(hdr, HTTP::EConnection, KKeepAlive);
    SetHeaderL(hdr, HTTP::ECacheControl, KNoCache);
    
    if (iRequest->type != NEREQT_IMAGE)
        SetHeaderL(hdr, HTTP::EAcceptEncoding, KGzipDeflateStr);
        
    if (iRequest->body) {
        TBuf8<8> length;
        length.Format(_L8("%d"), iBodyLen + iBodyFileLen);
        RStringPool stringPool = m_transaction->Session().StringPool();
        RStringF field = stringPool.StringF(HTTP::EContentLength, RHTTPSession::GetTable());
        hdr.SetRawFieldL(field, length, KFieldSeparator);
        if (iRequest->enc == NEENCT_URLENCODED)
            SetHeaderL(hdr, HTTP::EContentType, KTypeUrlencoded);
        else if (iRequest->enc == NEENCT_MULTIPART) {
            char* type = multipart_contentType();
            RStringF typeHdr = stringPool.StringF(HTTP::EContentType, RHTTPSession::GetTable());
            TPtrC8 typePtr((TUint8*)type);
            hdr.SetRawFieldL(typeHdr, typePtr, KFieldSeparator);
            NBK_free(type);
        }
    }
    
    if (iRequest->referer) {
        TPtrC8 ref((TUint8*)iRequest->referer);
        SetHeaderL(hdr, HTTP::EReferer, ref);
    }
}

void CHttpConn::RC_Submit()
{
    iComprLen = 0;
    iPostPos = 0;
    iRedirect = false;
    iCacheId = -1;
    iConnState = EActive;
    iSubmitTimes++;
    
    NCacheMgr* cacheMgr = iManager->GetCacheMgr();
    uint8 type;
    uint8 encoding;
    int length = 0;
    int idx = -1;
    nid mime = NEMIME_UNKNOWN;
    bool gzip = false;
    
    if (   iRequest->url
        && (iRequest->type == NEREQT_IMAGE || iRequest->type == NEREQT_CSS) ) {
        idx = cacheMgr_find(cacheMgr, iRequest->url, &type, &encoding, &length);
        if (idx != -1) {
            switch (type) {
            case NECACHET_WMLC:
                mime = NEMIME_DOC_WMLC;
                break;

            case NECACHET_HTM:
                mime = NEMIME_DOC_HTML;
                break;

            case NECACHET_HTM_GZ:
                mime = NEMIME_DOC_HTML;
                gzip = true;
                break;

            case NECACHET_CSS:
                mime = NEMIME_DOC_CSS;
                break;

            case NECACHET_CSS_GZ:
                mime = NEMIME_DOC_CSS;
                gzip = true;
                break;

            case NECACHET_JS:
                break;

            case NECACHET_JS_GZ:
                break;

            case NECACHET_GIF:
                mime = NEMIME_IMAGE_GIF;
                break;

            case NECACHET_JPG:
                mime = NEMIME_IMAGE_JPEG;
                break;

            case NECACHET_PNG:
                mime = NEMIME_IMAGE_PNG;
                break;

            default:
                mime = NEMIME_UNKNOWN;
            }
        }
    }
    
    // 处理图片请求
    if (iRequest->type == NEREQT_IMAGE) {
        // 检测本地文本缓存
        if (idx != -1) {
            iSimRequest = CSimRequest::NewL(CSimRequest::EFileCache);
            iSimRequest->SetObserver(this);
            iSimRequest->SetNbkRequest(iRequest);
            iSimRequest->SetFsCacheIndex(idx);
            iSimRequest->SetCacheMgr(cacheMgr);
            iSimRequest->SetMime(mime);
            iSimRequest->SetCompressed(gzip);
            iSimRequest->SetSize(length);
            iSimRequest->StartL();
            return;
        }
        else if (iRequest->method == NEREM_JUST_CACHE) {
            iSimRequest = CSimRequest::NewL(CSimRequest::ESimNoCacheError);
            iSimRequest->SetObserver(this);
            iSimRequest->SetNbkRequest(iRequest);
            iSimRequest->StartL();
            return;
        }
    }
    else if (iRequest->type == NEREQT_CSS) {
        // 检测本地文本缓存
        if (idx != -1) {
            iSimRequest = CSimRequest::NewL(CSimRequest::EFileCache);
            iSimRequest->SetObserver(this);
            iSimRequest->SetNbkRequest(iRequest);
            iSimRequest->SetFsCacheIndex(idx);
            iSimRequest->SetCacheMgr(cacheMgr);
            iSimRequest->SetMime(mime);
            iSimRequest->SetCompressed(gzip);
            iSimRequest->SetSize(length);
            iSimRequest->StartL();
            return;
        }
    }
    
    if (iRequest->method == NEREM_HISTORY) {
        iSimRequest = CSimRequest::NewL(CSimRequest::EHistoryCache);
        iSimRequest->SetObserver(this);
        iSimRequest->SetNbkRequest(iRequest);
        iSimRequest->StartL();
        return;
    }
    
    // 联网初始化
    CResourceManager* resMgr = CResourceManager::GetPtr();
    NBK_PlatformData* pfd = (NBK_PlatformData*)iRequest->platform;
    if (pfd) {
        CResourceManager::EHttpSessionState sessionState = resMgr->OpenHttpSessionIfNeededL(pfd->nbk);
        if (sessionState == CResourceManager::ENotReady) {
            iConnState = EWaiting;
            return;
        }
        else if (sessionState == CResourceManager::ENotSetIAP) {
            iConnState = EFinished;
            iSimRequest = CSimRequest::NewL(CSimRequest::ESimApError);
            iSimRequest->SetObserver(this);
            iSimRequest->SetNbkRequest(iRequest);
            iSimRequest->StartL();
            return;
        }
    }
    
    m_transaction = new (ELeave) RHTTPTransaction;
    RHTTPSession& session = resMgr->httpSession();
    RStringPool stringPool = session.StringPool();
    const TStringTable& stringTable = RHTTPSession::GetTable();
    
    TPtrC8 urlPtr;
    TUriParser8 uriParser;
    RStringF method;
    
    iBodyLen = iBodyFileLen = 0;
    TBool compressBody = EFalse;
    
    // 标准请求
    urlPtr.Set((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
    uriParser.Parse(urlPtr);
    // fragment
    if (uriParser.IsPresent(EUriFragment)) {
        m_frag = uriParser.Extract(EUriFragment).AllocL();
        // url without frag
        uriParser.UriWithoutFragment(urlPtr);
        // and reparse url
        uriParser.Parse(urlPtr);
    }

    if (iRequest->body) {
        iBodyLen = nbk_strlen(iRequest->body);
        method = stringPool.StringF(HTTP::EPOST, stringTable);
    }
    else
        method = stringPool.StringF(HTTP::EGET, stringTable);
    
    *m_transaction = session.OpenTransactionL(uriParser, *this, method);
    
    AddCookiesL(*m_transaction);
    
    if (iRequest->body) {
        m_transaction->Request().SetBody(*this);
        iBodyLen = nbk_strlen(iRequest->body);
        iBodyFileLen = GenerateUploadData(ETrue);
    }
    
    // 增加http头字段
    AddHeaders();

    // 启动连接
    m_transaction->SubmitL();
}

// 中止连接
void CHttpConn::RC_Cancel()
{
    if (m_isDone) return;
    m_isDone = true;
    
    if (iSimRequest) {
        iSimRequest->RC_Cancel();
        return;
    }

    if (m_transaction) {
        m_transaction->Cancel();
        m_transaction->Close();
        delete m_transaction;
        m_transaction = NULL;
    }
    
    CacheClose();
    
    if (iCacheId != -1) {
        NCacheMgr* cacheMgr = iManager->GetCacheMgr();
        cacheMgr_putData(cacheMgr, iCacheId, NULL, 0, N_TRUE);
    }
    
    iConnState = EFinished;
}

void CHttpConn::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent &aEvent)
{
    switch( aEvent.iStatus ) {
    
    case THTTPEvent::ERedirectedPermanently:
    case THTTPEvent::ERedirectedTemporarily:
    case THTTPEvent::ERedirectRequiresConfirmation:
    {
        // 302 跳转，转入标准处理
        iRequest->via = NEREV_STANDARD;
        CheckCookiesL(aTransaction);
        iRedirect = true;
        break;
    }
    
    case THTTPEvent::EGotResponseHeaders:
    {
        if (m_isDone) return;

        NRespHeader header;
        NBK_memset(&header, 0, sizeof(NRespHeader));
        header.content_length = -1;
        
        CheckGzipL(aTransaction);
        CheckCookiesL(aTransaction);
        
        int httpStatus(aTransaction.Response().StatusCode());
        
        if (httpStatus == KErrCompletion) {
            return;
        }

        if (httpStatus >= 300 && httpStatus < 400) {
            iRequest->via = NEREV_STANDARD;
            HandleRedirectEvent(THTTPEvent::ERedirectedPermanently, httpStatus);
            return;
        }
        else if (httpStatus >= 400) {
            Complete(httpStatus);
            return;
        }
        

        // 获取回应url
        m_urlResponse = NULL;
        if (m_frag) {
            // 补全 URL 后的页内偏移信息
            m_urlResponse = HBufC8::NewL(aTransaction.Request().URI().UriDes().Length() + m_frag->Length() + 1);
            TPtr8 responsePtr(m_urlResponse->Des());
            responsePtr.Copy(m_transaction->Request().URI().UriDes());
            responsePtr.Append(_L("#"));
            responsePtr.Append(*m_frag);
        }
        else {
            m_urlResponse = HBufC8::NewL(m_transaction->Request().URI().UriDes().Length());
            m_urlResponse->Des().Copy(m_transaction->Request().URI().UriDes());
        }
        
        if (MainDoc()) {
            // 获取回应URL
            
            bool replace = false;
            TPtr8 urlP = m_urlResponse->Des();
            
            if (iRequest->url) {
                TPtrC8 origUrl((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
                if (origUrl != urlP)
                    replace = true;
            }
            else
                replace = true;
            
            if (replace) {
                int size = urlP.Length() + 1;
                char* url = (char*)NBK_malloc(size);
                TPtr8 newUrl((TUint8*)url, 0, size);
                newUrl.Copy(urlP);
                url[size - 1] = 0;
                loader_setRequestUrl(iRequest, url, N_TRUE);
            }
        }
        
        // 获取头信息
        
        THTTPHdrVal hdrVal;
        RHTTPHeaders httpHeaders = m_transaction->Response().GetHeaderCollection();
        RStringPool stringPool = m_transaction->Session().StringPool();
        const TStringTable& stringTable = RHTTPSession::GetTable();
        
        // 获取 Content-Type
        if (httpHeaders.GetField(stringPool.StringF(HTTP::EContentType, stringTable), 0, hdrVal) == KErrNone) {
            TPtrC8 mime(hdrVal.StrF().DesC());
            
            if (mime.Find(_L8("wap.wmlc")) != KErrNotFound)
                header.mime = NEMIME_DOC_WMLC;
            else if (mime.Find(_L8("wap")) != KErrNotFound)
                header.mime = NEMIME_DOC_WML;
            else if (mime.Find(_L8("xhtml")) != KErrNotFound)
                header.mime = NEMIME_DOC_XHTML;
            else if (mime.Find(_L8("html")) != KErrNotFound)
                header.mime = NEMIME_DOC_HTML;
            else if (mime.Find(_L8("gif")) != KErrNotFound)
                header.mime = NEMIME_IMAGE_GIF;
            else if (mime.Find(_L8("jpeg")) != KErrNotFound)
                header.mime = NEMIME_IMAGE_JPEG;
            else if (mime.Find(_L8("png")) != KErrNotFound)
                header.mime = NEMIME_IMAGE_PNG;
            else if (mime.Find(_L8("css")) != KErrNotFound)
                header.mime = NEMIME_DOC_CSS;
        }

        // 获取 charset
        if( httpHeaders.GetParam(
                stringPool.StringF(HTTP::EContentType, stringTable ),
                stringPool.StringF(HTTP::ECharset, stringTable),
                hdrVal) == KErrNone )
        {
            m_encoding = hdrVal.StrF().DesC().AllocL();
            m_encoding->Des().LowerCase();
            if (   m_encoding->Des().Find(_L8("gbk")) != KErrNotFound
                || m_encoding->Des().Find(_L8("gb2312")) != KErrNotFound )
                header.encoding = NEENC_GB2312;
        }
        
        bool saveCache = false;
        
        if (iRedirect)
            saveCache = false;
        // 缓存图片
        else if (   iRequest->type == NEREQT_IMAGE
            && (   header.mime == NEMIME_IMAGE_GIF
                || header.mime == NEMIME_IMAGE_JPEG
                || header.mime == NEMIME_IMAGE_PNG ) )
            saveCache = true;
        // 缓存CSS
        else if (iRequest->type == NEREQT_CSS && header.mime == NEMIME_DOC_CSS)
            saveCache = true;
        
        if (saveCache) {
            NCacheMgr* cacheMgr = iManager->GetCacheMgr();
            bool gzip = (iNgz) ? true : false;
            uint8 type;
            uint8 encoding = header.encoding;
            
            switch (header.mime) {
            case NEMIME_DOC_WMLC:
                type = NECACHET_WMLC;
                break;

            case NEMIME_DOC_WML:
            case NEMIME_DOC_XHTML:
            case NEMIME_DOC_HTML:
                type = (gzip) ? NECACHET_HTM_GZ : NECACHET_HTM;
                break;

            case NEMIME_DOC_CSS:
                type = (gzip) ? NECACHET_CSS_GZ : NECACHET_CSS;
                break;

            case NEMIME_IMAGE_GIF:
                type = NECACHET_GIF;
                break;

            case NEMIME_IMAGE_JPEG:
                type = NECACHET_JPG;
                break;

            case NEMIME_IMAGE_PNG:
                type = NECACHET_PNG;
                break;

            default:
                type = NECACHET_UNKNOWN;
            }
            
            iCacheId = cacheMgr_store(cacheMgr, iRequest->url, type, encoding);
        }
        
        // 获取 Content-Length
        m_maxSize = -1;
        if( httpHeaders.GetField(stringPool.StringF( HTTP::EContentLength, stringTable ), 0, hdrVal ) == KErrNone ) {
            m_maxSize = hdrVal.Int();
            header.content_length = m_maxSize;
        }
        
        if (header.mime == NEMIME_UNKNOWN) {
            if (iRequest->type == NEREQT_MAINDOC) {
                RC_Cancel();
                CResourceManager::Download(this, m_urlResponse, NULL, NULL, m_maxSize);
                CResourceManager::OnCancel(this);
            }
            else {
                Complete(-113);
            }
            return;
        }
        
        if (iRequest->type == NEREQT_MAINDOC) {
            switch (header.mime) {
            case NEMIME_IMAGE_GIF:
            case NEMIME_IMAGE_JPEG:
            case NEMIME_IMAGE_PNG:
            {
                RC_Cancel();
                // 当请求主文档为图片时，将该图片包装成页面
                int size = nbk_strlen(iRequest->url);
                char* html = (char*)NBK_malloc(size + 128);
                size = sprintf(html, "<!doctype xhtml \"DTD/xhtml-mobile10.dtd\"><html><body><img src='%s'/>", iRequest->url);
                header.mime = NEMIME_DOC_XHTML;
                header.content_length = size;
                header.encoding = NEENC_UTF8;
                CResourceManager::OnReceiveHeader(this, &header);
                CResourceManager::OnReceiveData(this, html, size, size);
                CResourceManager::OnComplete(this);
                CreateCacheFile(header.mime);
                TPtrC8 chunkPtr((uint8*)html, size);
                Cache(chunkPtr);
                CacheClose();
                NBK_free(html);
                return;
            }
            } // switch
        }
        
        CResourceManager::OnReceiveHeader(this, &header);
        CreateCacheFile(header.mime);
        break;
    }
    
    case THTTPEvent::EGotResponseBodyData:
    {
        MHTTPDataSupplier* body;
        TPtrC8 chunkPtr;

        body = aTransaction.Response().Body();
        body->GetNextDataPart(chunkPtr);

        if (m_isDone) {
            body->ReleaseData();
            return;
        }
        Cache(chunkPtr);
        
        if (iCacheId != -1) {
            NCacheMgr* cacheMgr = iManager->GetCacheMgr();
            cacheMgr_putData(cacheMgr, iCacheId, (char*)chunkPtr.Ptr(), chunkPtr.Length(), N_FALSE);
        }
        
        if (iNgz) {
            body->GetNextDataPart(chunkPtr);
            iComprLen += chunkPtr.Length();
            
            ngzip_addZippedData(iNgz, (uint8*)chunkPtr.Ptr(), chunkPtr.Length());
            int unzipLen;
            char* unzip = (char*)ngzip_getData(iNgz, &unzipLen);
            if (unzip) {
                CResourceManager::OnReceiveData(this, unzip, unzipLen, iComprLen);
                iComprLen = 0;
            }
            body->ReleaseData();
        }
        else {
            // get it from the transaction
            body->GetNextDataPart(chunkPtr);
            if (chunkPtr.Length()) {
                CResourceManager::OnReceiveData(this,
                    (char*)chunkPtr.Ptr(), chunkPtr.Length(), chunkPtr.Length());
            }
            body->ReleaseData();
        }
        break;
    }
    
    case THTTPEvent::ERequestComplete:
        break;
   
    case THTTPEvent::EResponseComplete:
        break;
        
    case THTTPEvent::ESucceeded:
        Complete(KErrNone);
        break;
        
    case THTTPEvent::EFailed:
    case THTTPEvent::EMoreDataReceivedThanExpected:
    case THTTPEvent::EUnrecoverableError:
    case KErrAbort:
    {
        if (!m_transaction)
            return;
        
        int statusCode = m_transaction->Response().StatusCode();
        if ((statusCode == 404) && (aEvent.iStatus == THTTPEvent::EFailed)) {
            Complete(KErrNone);
        }   
        else if (statusCode != 200) {
            Complete(-25000 - m_transaction->Response().StatusCode());
        }
        else if (statusCode == 200 && aEvent.iStatus == THTTPEvent::EFailed) {
            // this is a weird combination, it is caused by cached supplier when a response only has the
            // response header, but no response body is sent by the HTTP stack.  It is a legitimate senario though.
            Complete(KErrNone);
        }
        else {
            Complete(aEvent.iStatus);
        }
        
        break;
    }
    
    default:
        Complete(aEvent.iStatus);
        break;
    }
}

TInt CHttpConn::MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent &aEvent)
{
    Complete(aError);
    return KErrNone;
}

void CHttpConn::Complete(int error)
{
    if (m_isDone) return;
    m_isDone = true;
    
    if (m_transaction) {
        m_transaction->Cancel();
        m_transaction->Close();
        delete m_transaction;
        m_transaction = NULL;
    }

    CacheClose();
   
    if (error == KErrNone) {
        iConnState = MResConn::EFinished;
        CResourceManager::OnComplete(this);
    }
    else {
        if (KErrNotReady == error) {
            iConnState = MResConn::EWaiting;
            CResourceManager* resMgr = CResourceManager::GetPtr();           
            NBK_PlatformData* pfd = (NBK_PlatformData*)iRequest->platform;
            resMgr->OnHTTPIapNotReady(pfd->nbk);
        }
        else {
            iConnState = MResConn::EFinished;
            CResourceManager::OnError(this, error);
        }
    }
    
    if (iCacheId != -1) {
        NCacheMgr* cacheMgr = iManager->GetCacheMgr();
        cacheMgr_putData(cacheMgr, iCacheId, NULL, 0, N_TRUE);
    }
}

int CHttpConn::HandleRedirectEvent(int event, int aHttpStatus)
{  
    RHTTPSession& session = iManager->httpSession();
    RStringF location = session.StringPool().StringF(HTTP::ELocation, RHTTPSession::GetTable());
    RHTTPHeaders responseHeaders(m_transaction->Response().GetHeaderCollection());
    THTTPHdrVal locationValue;
    TUriParser8 uriParser;
    int ret(KErrNone);

    if (responseHeaders.GetField(location, 0, locationValue) == KErrNone) {
        // 拼装新地址
        int len = locationValue.StrF().DesC().Length();
        char* loc = (char*)NBK_malloc(len + 1);
        nbk_strncpy(loc, (char*)locationValue.StrF().DesC().Ptr(), len);
        char* newUrl = url_parse(iRequest->url, loc);
        NBK_free(loc);
        char* referer = str_clone(iRequest->url);
        loader_setRequestUrl(iRequest, newUrl, N_TRUE);
        loader_setRequestReferer(iRequest, referer, N_TRUE);
        TPtrC8 urlPtr((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
        uriParser.Parse(urlPtr);
        if (m_frag) {
            delete m_frag;
            m_frag = NULL;
        }

        // 重置
        m_transaction->Cancel();
        m_transaction->Request().RemoveBody();
        m_transaction->Response().RemoveBody();
        m_transaction->Request().GetHeaderCollection().RemoveAllFields();
        
        // 检测 Oauth2协议
        if (   aHttpStatus == 302
            && nbk_strfind(iRequest->referer, "/oauth2/authorize") > 0 ) {
            TPtrC8 resp((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
            CNBrKernel* nbk = (CNBrKernel*)((NBK_PlatformData*)iRequest->platform)->nbk;
            nbk->GetShellInterface()->OnOauth2Response(resp);
            return ret;
        }
        
        // 设置新地址
        m_transaction->Request().SetURIL(uriParser);
        AddCookiesL(*m_transaction);
        AddHeaders();
        RStringF method = session.StringPool().StringF(HTTP::EGET, RHTTPSession::GetTable());
        m_transaction->Request().SetMethod(method);
        TRAP(ret, m_transaction->SubmitL());
    }
    else
        ret = KErrCancel;
    
    return ret;
}

TBool CHttpConn::IsNonHttpRedirect()
{
    TUriParser8 uriParser;
    if (uriParser.Parse(m_transaction->Request().URI().UriDes()) == KErrNone) {
        if (uriParser.IsPresent(EUriHost) && uriParser.IsPresent(EUriScheme)) {
            const TDesC8& scheme = uriParser.Extract(EUriScheme);
            if (scheme.FindF(_L8("http")) == KErrNotFound)
                return ETrue;
        }
    }
    return EFalse;
}

void CHttpConn::CheckGzipL(const RHTTPTransaction& aTrans)
{
    // read the header data and check the MIME type here    
    // check the status and body
    RStringPool stringPool = aTrans.Session().StringPool();
    RHTTPResponse response = aTrans.Response();
    RHTTPHeaders headers = response.GetHeaderCollection();
    RStringF fieldNameStr = stringPool.StringF(HTTP::EContentEncoding, RHTTPSession::GetTable());

    // read the first part of content-encoding field
    THTTPHdrVal fieldVal;
    if (headers.GetField(fieldNameStr, 0, fieldVal) == KErrNone) {
        if ((fieldVal.StrF().Index(RHTTPSession::GetTable()) == HTTP::EGzip ||
             fieldVal.StrF().Index(RHTTPSession::GetTable()) == HTTP::EDeflate)) {
            CreateNgzip();
        }
    }
}

void CHttpConn::AddCookiesL(RHTTPTransaction& aTransaction)
{
    const TUriC8& requestUri = aTransaction.Request().URI();
    
    TPtrC8 host(requestUri.Extract(EUriHost));
    char* hostC = (char*)NBK_malloc(host.Length() + 1);
    nbk_strncpy(hostC, (char*)host.Ptr(), host.Length());
    *(hostC + host.Length()) = 0;

    char* pathC;
    TPtrC8 path(requestUri.Extract(EUriPath));
    if (path.Length()) {
        pathC = (char*)NBK_malloc(path.Length() + 1);
        nbk_strncpy(pathC, (char*)path.Ptr(), path.Length());
        *(pathC + path.Length()) = 0;
    }
    else {
        pathC = (char*)NBK_malloc(2);
        *pathC = '/';
        *(pathC + 1) = 0;
    }
    
    NCookieMgr* cookieMgr = iManager->GetCookieManager();
    char* cookies = cookieMgr_getCookie(cookieMgr, hostC);
    if (cookies) {
        RStringPool stringPool = aTransaction.Session().StringPool();
        RStringF cookieHdr = stringPool.StringF(HTTP::ECookie, RHTTPSession::GetTable());
        TPtrC8 cookiesPtr((TUint8*)cookies);
        RHTTPHeaders hdr = aTransaction.Request().GetHeaderCollection();
        hdr.SetRawFieldL(cookieHdr, cookiesPtr, KFieldSeparator);
        NBK_free(cookies);
    }
    
    NBK_free(hostC);
    NBK_free(pathC);
}

void CHttpConn::CheckCookiesL(RHTTPTransaction& aTransaction)
{
    RStringPool stringPool = aTransaction.Session().StringPool();
    const TStringTable& stringTable = RHTTPSession::GetTable();
    RHTTPHeaders responseHeaders = aTransaction.Response().GetHeaderCollection();

    RArray<RStringF> fields;
    fields.Append(stringPool.StringF(HTTP::ESetCookie, stringTable));
    fields.Append(stringPool.StringF(HTTP::ECookieName, stringTable));
    fields.Append(stringPool.StringF(HTTP::ECookieValue, stringTable));
    fields.Append(stringPool.StringF(HTTP::EExpires, stringTable));
    fields.Append(stringPool.StringF(HTTP::EPath, stringTable));
    fields.Append(stringPool.StringF(HTTP::EDomain, stringTable));
    
    TInt numCookies = responseHeaders.FieldPartsL(fields[0]);
    if (numCookies) {
        char *hostC, *lineC, *p;

        const TUriC8& requestUri = aTransaction.Request().URI();
        TPtrC8 host(requestUri.Extract(EUriHost));
        hostC = (char*)User::Alloc(host.Length() + 1);
        strncpy(hostC, (char*)host.Ptr(), host.Length());
        *(hostC + host.Length()) = 0;
        
        THTTPHdrVal val;
        TPtrC8 valP;
        lineC = (char*)User::Alloc(KMaxCookieLineLength);
        
        NCookieMgr* cookieMgr = iManager->GetCookieManager();
        
        for (int i=0; i < numCookies; i++) {
            p = lineC;
            for (int j=1; j < 6; j++) {
                if (responseHeaders.GetParam(fields[0], fields[j], val, i) == KErrNone) {
                    
                    switch (j) {
                    case 3:
                        strcpy(p, "expires=");
                        p += 8;
                        break;
                    case 4:
                        strcpy(p, "path=");
                        p += 5;
                        break;
                    case 5:
                        strcpy(p, "domain=");
                        p += 7;
                        break;
                    }
                    
                    if (j < 3)
                        valP.Set(val.Str().DesC());
                    else
                        valP.Set(val.StrF().DesC());
                    strcpy(p, (char*)valP.Ptr());
                    p += valP.Length();
                    
                    if (j == 1)
                        *p++ = '=';
                    else {
                        *p++ = ';';
                        *p++ = ' ';
                    }
                }
            }
            *p = 0;

            cookieMgr_setCookie(cookieMgr, hostC, lineC);
        }
        
        int httpStatus(aTransaction.Response().StatusCode());
        User::Free(lineC);       
        User::Free(hostC);
    }
    
    fields.Close();
}

// 获取下一块post数据
TBool CHttpConn::GetNextDataPart(TPtrC8& aDataPart)
{
    const uint8* body;
    int rest;
    TBool ret;
    
    if (iRequest->body) {
        body = (uint8*)iRequest->body;
    }
    
    if (iPostPos >= iBodyLen + iBodyFileLen) {
        aDataPart.Set(KNullDesC8);
        ret = ETrue;
    }
    else {
        iSent = KMaxPostBufSize;
        if (iBodyFileLen && iPostPos >= iBodyLen) {
            int pos = iPostPos - iBodyLen;
            rest = iBodyFileLen - pos;           
            iSent = (iSent <= rest) ? iSent : rest;
            
            if (iPostFileBuf == NULL)
                iPostFileBuf = HBufC8::New(KMaxPostBufSize);
            
            TPtr8 bufPtr(iPostFileBuf->Des());
            iPostFile->Seek(ESeekStart, pos);
            iPostFile->Read(bufPtr, iSent);
            aDataPart.Set(iPostFileBuf->Des());
        }
        else {
            rest = iBodyLen - iPostPos;
            iSent = (iSent <= rest) ? iSent : rest;
            aDataPart.Set(body + iPostPos, iSent);
        }
        
        ret = (iPostPos + iSent >= iBodyLen + iBodyFileLen) ? ETrue : EFalse;
    }
    
    return ret;
}

// 释放上一次post的数据
void CHttpConn::ReleaseData()
{
    iPostPos += iSent;
    
    if (iPostPos < iBodyLen + iBodyFileLen) {
        m_transaction->NotifyNewRequestBodyPartL(); // 通知post数据未处理完
    }
    else {
        CResourceManager::OnWaiting(this);

        // 释放临时数据
        if (iPostFileBuf) {
            delete iPostFileBuf;
            iPostFileBuf = NULL;
        }
        
        if (iPostFile) {
            iPostFile->Close();
        }
    }
}

TInt CHttpConn::OverallDataSize()
{
    return iBodyLen + iBodyFileLen;
}

TInt CHttpConn::Reset()
{
    iPostPos = 0;
    return KErrNone;
}

// 创建数据资源缓存，用于历史操作
void CHttpConn::CreateCacheFile(int aMime)
{
    if (iCacheFile == NULL) {
        int len = iManager->GetCachePath().Length();
        iCacheFile = HBufC::New(len + 64);
    }
    iCacheFile->Des().Zero();
    
    if (iBufFileWriter == NULL)
        iBufFileWriter = CBufFileWriter::NewL();
    
    if (MainDoc())
        iManager->CleanCachedPage(iRequest->pageId);
    
    if (MainDoc() && m_urlResponse) {
        // 保存URI
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        p.Format(KCacheUriFormat, &path, iRequest->pageId);
        if (iBufFileWriter) {
            iBufFileWriter->Open(p);
            iBufFileWriter->Write(m_urlResponse->Des());
            iBufFileWriter->Close();
        }
    }
    
    // 标准页面
    if (   MainDoc()
        && (   aMime == NEMIME_DOC_HTML
            || aMime == NEMIME_DOC_XHTML
            || aMime == NEMIME_DOC_WML ) ) {
        
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        p.Format(KCacheDocFormat, &path, iRequest->pageId);
        if (iNgz)
            p.Append(KGzipSuffix);
        else
            p.Append(KHtmlSuffix);
        
        if (iBufFileWriter)
            iBufFileWriter->Open(p);
        
#if DEBUG_CACHE
        CProbe* pb = CResourceManager::GetProbe();
        pb->OutputChar("create doc", -1);
        pb->OutputReturn();
#endif
    }
    // wmlc
    else if (MainDoc() && aMime == NEMIME_DOC_WMLC) {
        
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        p.Format(KCacheDocFormat, &path, iRequest->pageId);
        p.Append(KWbxmlSuffix);
        
        if (iBufFileWriter)
            iBufFileWriter->Open(p);
    }
    // 标准图片
    else if (   aMime == NEMIME_IMAGE_GIF
             || aMime == NEMIME_IMAGE_JPEG
             || aMime == NEMIME_IMAGE_PNG )
    {
        TPtrC8 url;
        TBuf8<32> md5str;
        TBuf<32> md5strU;
        url.Set((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
        CResourceManager::GetMsgDigestByMd5L(md5str, url);
        md5strU.Copy(md5str);
        
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        p.Format(KCachePathFormat, &path, iRequest->pageId);
        p.Append(md5strU);
        
        if (iBufFileWriter)
            iBufFileWriter->Open(p);
        
#if DEBUG_CACHE
        CProbe* pb = CResourceManager::GetProbe();
        pb->OutputChar("create image", -1);
        pb->OutputReturn();
#endif
    }
    // CSS
    else if (aMime == NEMIME_DOC_CSS) {
        TPtrC8 url;
        TBuf8<32> md5str;
        TBuf<32> md5strU;
        url.Set((TUint8*)iRequest->url, nbk_strlen(iRequest->url));
        CResourceManager::GetMsgDigestByMd5L(md5str, url);
        md5strU.Copy(md5str);
        
        TPtr p = iCacheFile->Des();
        TPtr path = iManager->GetCachePath();
        p.Format(KCachePathFormat, &path, iRequest->pageId);
        p.Append(md5strU);
        if (iNgz)
            p.Append(KCszSuffix);
        else
            p.Append(KCssSuffix);
        
        if (iBufFileWriter)
            iBufFileWriter->Open(p);
    }
}

// 缓存数据，用于历史操作
void CHttpConn::Cache(TPtrC8& aData)
{
    if (iCacheFile && iCacheFile->Length()) {
        if (iBufFileWriter)
            iBufFileWriter->Write(aData);
    }
}

void CHttpConn::CacheClose()
{
    if (iCacheFile) {
        iCacheFile->Des().Zero();
        if (iBufFileWriter)
            iBufFileWriter->Close();
    }
}

// 接收来自模拟数据请求发来的回应事件
void CHttpConn::OnSimRequestData(MSimRequestObserver::TStatus aStatus)
{
    switch (aStatus) {
    case MSimRequestObserver::ESimReqHeader:
    {
        NRespHeader header;
        NBK_memset(&header, 0, sizeof(NRespHeader));
        header.mime = iSimRequest->Mime();
        header.content_length = iSimRequest->GetSize();
        
        HBufC8* url = iSimRequest->GetUrl();
        if (url) {
            TPtr8 urlP = url->Des();
            int size = urlP.Length() + 1;
            char* url = (char*)NBK_malloc(size);
            TPtr8 newUrl((TUint8*)url, 0, size);
            newUrl.Copy(urlP);
            url[size - 1] = 0;
            loader_setRequestUrl(iRequest, url, 1);
        }
        
        CResourceManager::OnReceiveHeader(this, &header);
        
        if (iSimRequest->IsCompressed()) {
            CreateNgzip();
        }
        break;
    }
    case MSimRequestObserver::ESimReqData:
    {
        TPtrC8 chunkPtr;
        
        if (iNgz) {
            iSimRequest->GetNextDataPart(chunkPtr);
            iComprLen += chunkPtr.Length();
            
            ngzip_addZippedData(iNgz, (uint8*)chunkPtr.Ptr(), chunkPtr.Length());
            int unzipLen;
            char* unzip = (char*)ngzip_getData(iNgz, &unzipLen);
            if (unzip) {
                CResourceManager::OnReceiveData(this, unzip, unzipLen, iComprLen);
                iComprLen = 0;
            }
            iSimRequest->ReleaseData();
        }
        else {
            iSimRequest->GetNextDataPart(chunkPtr);
            if (chunkPtr.Length()) {
                CResourceManager::OnReceiveData(this,
                    (char*)chunkPtr.Ptr(), chunkPtr.Length(), chunkPtr.Length());
            }
            iSimRequest->ReleaseData();
        }
        break;
    }
    case MSimRequestObserver::ESimReqComplete:
    {
        if (!m_isDone) {
            m_isDone = true;
            iConnState = EFinished;
            CResourceManager::OnComplete(this);
        }
        break;
    }
    case MSimRequestObserver::ESimReqError:
    {
        if (!m_isDone) {
            m_isDone = true;
            iConnState = EFinished;
            CResourceManager::OnError(this, NELERR_NOCACHE);
        }
        break;
    }
    case MSimRequestObserver::ESimReqNoCache:
    {
        if (!m_isDone) {
            m_isDone = true;
            iConnState = EFinished;
            CResourceManager::OnError(this, NELERR_NOCACHE);
        }
        break;
    }
    }
}

void CHttpConn::OnIapFailed(void)
{
    // 当选择接入点无效时，模拟发出一个下载出错通知
    if (iSimRequest) {
        delete iSimRequest;
        iSimRequest = NULL;
    }
    iSimRequest = CSimRequest::NewL(CSimRequest::ESimApError);
    iSimRequest->SetObserver(this);
    iSimRequest->SetNbkRequest(iRequest);
    iSimRequest->StartL();
}

// 按照上传文件列表，生成上传数据
int CHttpConn::GenerateUploadData(TBool aStandard)
{
    int length = 0;
    
    if (iRequest->files == N_NULL)
        return 0;
    
    RFs& rfs = CCoeEnv::Static()->FsSession();
    RFile file;
    int i;
    const int bufSize = 1024 * 16;
    HBufC8* bufH = HBufC8::New(bufSize);
    TBuf<64> postfn;

    iPageId = iRequest->pageId;
    postfn.Format(KPostFileStreamFmt, iPageId);
    rfs.Delete(postfn);
    
    // 将所有待上传文件存入临时缓存文件
    for (i=0; iRequest->files[i].name; i++) {
        
        if (iPostFile == NULL) {
            iPostFile = new RFile;
            if (iPostFile->Create(rfs, postfn, EFileWrite) != KErrNone) {
                delete iPostFile;
                iPostFile = NULL;
                break;
            }
        }

        int len, hdrLen;
        wchr* fn16 = uni_utf8_to_utf16_str((uint8*)iRequest->files[i].path, -1, &len);
        TPtrC fnPtr(fn16, len);
        TPtr8 dataPtr(NULL, 0, 0);
        
        if (file.Open(rfs, fnPtr, EFileRead) == KErrNone) {
            
            file.Size(len);
            length += len;
            
            // http 标准
            char* p = (char*)bufH->Ptr();
            hdrLen = multipart_file(iRequest->files[i].name, iRequest->files[i].path, p);
            length += hdrLen;
            dataPtr.Set((TUint8*)p, hdrLen, hdrLen);
            iPostFile->Write(dataPtr);
            
            // 将待上传文件内容写入缓存文件
            int read = 0;
            dataPtr.Set(bufH->Des());
            while (read < len) {
                file.Read(dataPtr);
                read += dataPtr.Length();
                iPostFile->Write(dataPtr);
            }
            file.Close();
        }
        NBK_free(fn16);
    }

    if (iPostFile) {
        if (aStandard) {
            char* p = (char*)bufH->Ptr();
            int len = multipart_end(p);
            length += len;
            iPostFile->Write(TPtrC8((TUint8*)p, len));
        }
        iPostFile->Flush();
    }
    
    delete bufH;
    
    return length;
}

void CHttpConn::CreateNgzip()
{
    if (iNgz)
        ngzip_delete(&iNgz);
    
    iNgz = ngzip_create();
}
