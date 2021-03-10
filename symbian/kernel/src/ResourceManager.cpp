/*
 ============================================================================
 Name		: ResourceManager.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CResourceManager implementation
 ============================================================================
 */

#include "../../../stdc/inc/config.h"
#include "../../../stdc/inc/nbk_version.h"
#include "../../../stdc/inc/nbk_cbDef.h"
#include "../../../stdc/loader/loader.h"
#include "../../../stdc/loader/url.h"
#include "../../../stdc/loader/upCmd.h"
#include "../../../stdc/render/imagePlayer.h"
#include "../../../stdc/tools/str.h"
#include "../../../stdc/tools/unicode.h"
#include "../../../stdc/dom/history.h"
#include <stringpool.h>
#include <rhttpconnectioninfo.h>
#include <rhttppropertyset.h>
#include <httpstringconstants.h>
#include <CommDbConnPref.h>
#include <rhttpsession.h>
#include <uri8.h>
#include <Etel3rdParty.h>
#include <hash.h>
#include <f32file.h>
#include <coemain.h>
#include <eikenv.h>
#include <bautils.h>
#include <nbk_core.mbg>
#include "ResourceManager.h"
#include "FileConn.h"
#include "HttpConn.h"
#include "NBrKernel.h"
#include "Probe.h"
#include "NBKPlatformData.h"
#include "NBrKernel.h"
#include "NetAddr.h"
#include "FEPHandler.h"
#include "DecodeThread.h"

#define DUMP_DOC        0
#define DEBUG_CONN_MGR  0

//#define TIME_LOG

_LIT(KCachePath, "c:\\data\\nbk\\");
_LIT(KDumpDataFile, "c:\\data\\nbk\\doc.htm");
_LIT(KDumpXmlFileFmt, "c:\\data\\nbk\\doc%02d.htm");

static char* l_cookieFilePath = "c:\\data\\nbk\\nbk_cookie.dat";

_LIT8(KHttpProtString, "HTTP/TCP" );
_LIT(KCMCCCmwap, "cmwap");
_LIT(KUniCmwap, "uniwap");
_LIT8(KHttpProxy, "10.0.0.172:80");

const nid KAllPageId = 0xffff;
const nid KUserPageId = 65000; // 用于特定的连接

static CResourceManager* l_resourceMananger = NULL;

////////////////////////////////////////////////////////////
//
// CAsyWait
//

class CAsyWait : public CActive
{
public:
    ~CAsyWait();
    static CAsyWait* NewL();

public:
    TRequestStatus& GetStatus();
    TInt StartWaitD();

private:
    CAsyWait();
    void ConstructL();

private:
    void RunL();
    void DoCancel();

private:
    CActiveSchedulerWait* iWaiter;
};

CAsyWait::CAsyWait() : CActive(EPriorityStandard)
{
}

CAsyWait* CAsyWait::NewL()
{
    CAsyWait* self = new (ELeave) CAsyWait();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
}

void CAsyWait::ConstructL()
{
    CActiveScheduler::Add(this);
    iWaiter = new (ELeave) CActiveSchedulerWait();
}

CAsyWait::~CAsyWait()
{
    Cancel();
    delete iWaiter;
    iWaiter = NULL;
}

void CAsyWait::DoCancel()
{
    iWaiter->AsyncStop();
}

void CAsyWait::RunL()
{
    iWaiter->AsyncStop();
}

TRequestStatus& CAsyWait::GetStatus()
{
    return iStatus;
}

TInt CAsyWait::StartWaitD()
{
    SetActive();
    iWaiter->Start();
    TInt err = iStatus.Int();
    delete this;
    return err;
}

////////////////////////////////////////////////////////////
//
// NBK 网络接口实现
//
// define in loader.h

// 是否由外部处理错误
nbool NBK_handleError(int error)
{
    return N_FALSE;
}

// 获取内置页面
char* NBK_getStockPage(int type)
{
    switch (type) {
    case NESPAGE_ERROR: // 通用错误页
        return "file://c/data/nbk_error.htm";

    case NESPAGE_ERROR_404: // 404 not found 错误页
        return "file://c/data/nbk_404.htm";

    default:
        return NULL;
    }
}

static void request_uri_add_suffix(NRequest* req, const TDesC8& aSuffix)
{
    int size = 4;
    TPtrC8 suff(aSuffix);

    size += suff.Length();
    size += nbk_strlen(req->url);

    char* url = (char*) NBK_malloc(size);
    char* q = url;
    nbk_strncpy(q, (char*) suff.Ptr(), suff.Length());
    q += suff.Length();
    nbk_strcpy(q, req->url);

    loader_setRequestUrl(req, url, N_TRUE); // 置换请求地址
    req->via = NEREV_STANDARD;
}

// 资源请求入口
nbool NBK_resourceLoad(void* pfd, NRequest* req)
{
    CResourceManager* resMgr = CResourceManager::GetPtr();

    if (req->via == NEREV_TF) {
        request_uri_add_suffix(req, KTfService);
    }

    if (req->method == NEREM_HISTORY) {
        CHttpConn* conn = CHttpConn::NewL(req);
        conn->SetManager(resMgr);
        resMgr->AddConn(conn);
        conn->RC_Submit();
        return N_TRUE;
    }
    else {
        if (   nbk_strncmp(req->url, "http://", 7) == 0
            || nbk_strncmp(req->url, "https://", 8) == 0 ) {
            CHttpConn* conn = CHttpConn::NewL(req);
            conn->SetManager(resMgr);
            resMgr->AddConn(conn);
            conn->RC_Submit();
            return N_TRUE;
        }
        else if (nbk_strncmp(req->url, "file://", 7) == 0) {
            CFileConn* conn = CFileConn::NewL(req);
            conn->SetManager(resMgr);
            resMgr->AddConn(conn);
            conn->RC_Submit();
            return N_TRUE;
        }
    }

    return N_FALSE;
}

// 停止指定页号的所有资源请求
void NBK_resourceStopAll(void* pfd, nid pageId)
{
    if (l_resourceMananger) {
        l_resourceMananger->StopAll(pageId);
    }
}

// define in history.h

// 清理指定页号的历史缓存数据
void NBK_resourceClean(void* pfd, nid pageId)
{
    if (l_resourceMananger) {
        l_resourceMananger->CleanCachedPage(pageId);
    }
}

void NBK_unlink(const char* path)
{
    RFs& fs = CCoeEnv::Static()->FsSession();
    TPtrC8 p8((TUint8*)path, nbk_strlen(path));
    HBufC* pa = HBufC::New(64);
    pa->Des().Copy(p8);
    fs.Delete(pa->Des());
    delete pa;
}

////////////////////////////////////////////////////////////
//
// CResourceManager 资源管理器，处理所有的资源请求
//

static char* CACHE_PATH     = "c:\\data\\nbk\\cache\\";
static char* COOKIE_PATH    = "c:\\data\\nbk\\cookies.dat";

CResourceManager::CResourceManager()
    : m_sessionRunning(false)
    , m_IAPSeted(false)
    , m_isWap(false)
    , iProbe(NULL)
    , iUserAgent(NULL)
    , iFileIdx(-1)
{
}

CResourceManager::~CResourceManager()
{
    if (iDecodeThread) {
        delete iDecodeThread;
        iDecodeThread = NULL; // it shoudn't be dandling in case of DLL load/unload at runtime         
    }

    iTimer->Cancel();
    delete iTimer;
    
    CleanAllConn(KAllPageId);
    CloseHttpSession();

    sll_delete(&iConnList);

    if (m_IAPSeted && m_bUseOutNetSettting) {
        iConnection = NULL;
        iSocketServ = NULL;
    }
    else if (m_IAPSeted && !m_bUseOutNetSettting) {
        m_connection.Close();
        m_socketServ.Close();
    }

    cookieMgr_save(iCookieMgr, COOKIE_PATH);
    cookieMgr_delete(&iCookieMgr);
    
    cacheMgr_save(iCacheMgr);
    cacheMgr_delete(&iCacheMgr);

    if (iUserAgent)
        delete iUserAgent;
    if (iImei)
        delete iImei;

    if (iCachePath)
        delete iCachePath;
    
    if (iProbe) {
        delete iProbe;
        iProbe = NULL;
    }
    if (iSysControl) {
        delete iSysControl;
        iSysControl = NULL;
    }
}

CResourceManager* CResourceManager::NewLC()
{
    CResourceManager* self = new (ELeave) CResourceManager();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CResourceManager* CResourceManager::NewL()
{
    if (l_resourceMananger == NULL) {
        l_resourceMananger = CResourceManager::NewLC();
        CleanupStack::Pop(); // self;
    }
    return l_resourceMananger;
}

void CResourceManager::ConstructL()
{
    iProbe = new CProbe();
    iTimer = CPeriodic::NewL(0);

    RFs& fs = CCoeEnv::Static()->FsSession();
    fs.MkDirAll(_L("c:\\data\\nbk\\cache\\"));
    
    iCookieMgr = cookieMgr_create();
    cookieMgr_load(iCookieMgr, COOKIE_PATH);
    
    iCacheMgr = cacheMgr_create(50);
    cacheMgr_setPath(iCacheMgr, CACHE_PATH);
    cacheMgr_load(iCacheMgr);
    
    iConnList = sll_create();

    GenPhoneID();
}

void CResourceManager::Destroy()
{
    if (l_resourceMananger) {
        delete l_resourceMananger;
        l_resourceMananger = NULL;
    }
}

void CResourceManager::GenPhoneID()
{
    CTelephony* telephony = CTelephony::NewLC();
    CAsyWait* wait = CAsyWait::NewL();
    CTelephony::TPhoneIdV1Pckg idV1Pkg(idV1);
    telephony->GetPhoneId(wait->GetStatus(), idV1Pkg);
    TInt err = wait->StartWaitD();
    if (err == KErrNone) {
        _LIT8(Kua, "Baidu-NBK/%d.%d (SymbianOS/9.2; Series60/3.2; %S/%S)");
        _LIT8(Kuid, "uid=nbk_%S");
        int size;

        TBuf8<CTelephony::KPhoneManufacturerIdSize> buf;
        TBuf8<CTelephony::KPhoneManufacturerIdSize> buf2;

        // 生成标准UA
        iUserAgent = HBufC8::New(256);
        buf.Copy(idV1.iManufacturer);
        buf2.Copy(idV1.iModel);
        iUserAgent->Des().Format(Kua, NBK_VER_MAJOR, NBK_VER_MINOR, &buf, &buf2);

//        // 生成百度用户标识
//        size = 8 + CTelephony::KPhoneSerialNumberSize;
//        iImei = HBufC8::New(size);
//        buf.Copy(idV1.iSerialNumber);
//        iImei->Des().Format(Kuid, &buf);
    }
    CleanupStack::PopAndDestroy();
}

void CResourceManager::ResendRequest()
{
    MResConn* c = (MResConn*) sll_first(iConnList);
    while (c) {
        if (MResConn::EWaiting == c->iConnState) {
            c->RC_Submit();
        }
        c = (MResConn*) sll_next(iConnList);
    }
}

void CResourceManager::OnReceiveHeader(MResConn* conn, NRespHeader* header)
{
    if (conn->MainDoc()) {
        CProbe::FileInit(KDumpDataFile);
    }

    loader_onReceiveHeader(conn->iRequest, header);
}

void CResourceManager::OnReceiveData(MResConn* conn, char* data, int length, int comprLen)
{
    if (conn->MainDoc()) {
        CProbe::FileWrite(KDumpDataFile, TPtrC8((TUint8*) data, length));
    }

    loader_onReceiveData(conn->iRequest, data, length, comprLen);
}

void CResourceManager::OnComplete(MResConn* conn)
{
    loader_onComplete(conn->iRequest);
    l_resourceMananger->StartClean();
}

void CResourceManager::OnCancel(MResConn* conn)
{
    loader_onCancel(conn->iRequest);
    l_resourceMananger->StartClean();
}

void CResourceManager::OnError(MResConn* conn, int err)
{
    loader_onError(conn->iRequest, err);
    l_resourceMananger->StartClean();
}

void CResourceManager::StartClean()
{
    if (!iTimer->IsActive()) {
        iTimer->Cancel();
        iTimer->Start(10, 5000000, TCallBack(OnCleanUnusedConn, this));
    }
}

TInt CResourceManager::OnCleanUnusedConn(TAny* ptr)
{
    CResourceManager* self = (CResourceManager*)ptr;
    self->iTimer->Cancel();
    self->CleanUnusedConn();
    return 0;
}

void CResourceManager::CleanUnusedConn()
{
    MResConn* c = (MResConn*) sll_first(iConnList);
    while (c) {
        if (c->iConnState == MResConn::EFinished /*|| c->IsStop()*/) {
            delete c;
            sll_removeCurr(iConnList);
        }
        c = (MResConn*) sll_next(iConnList);
    }
}

void CResourceManager::AddConn(MResConn* conn)
{
    sll_append(iConnList, conn);
}

void CResourceManager::CleanAllConn(nid aPageId)
{
    MResConn* c = (MResConn*) sll_first(iConnList);
    while (c) {
        if (aPageId == KAllPageId || c->PageId() == aPageId) {
            c->RC_Cancel();
            delete c;
            sll_removeCurr(iConnList);
        }
        c = (MResConn*) sll_next(iConnList);
    }
}

void CResourceManager::StopAll(nid aPageId)
{
    CleanAllConn(aPageId);
    StartClean();
}

CResourceManager* CResourceManager::GetPtr()
{
    NewL();
    return l_resourceMananger;
}

CResourceManager::EHttpSessionState CResourceManager::OpenHttpSessionIfNeededL(void* aKernel)
{
    if (!m_IAPSeted || !m_IAPReady) {
        ((CNBrKernel*)aKernel)->RequestAP();

        if (!m_IAPSeted)
            return ENotSetIAP;

        if (m_IAPSeted && !m_IAPReady)
            return ENotReady;
    }

    if (!m_sessionRunning) {

        m_httpSession.OpenL(KHttpProtString);
        m_sessionRunning = true;

        RStringPool strP = m_httpSession.StringPool();
        const TStringTable& stringTable = RHTTPSession::GetTable();
        RHTTPConnectionInfo connInfo = m_httpSession.ConnectionInfo();

        if (m_bUseOutNetSettting) {
            // 使用外壳提供的连接
            if (iSocketServ)
                connInfo.SetPropertyL(
                    strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()),
                    THTTPHdrVal(iSocketServ->Handle()) );
            if (iConnection)
                connInfo.SetPropertyL(
                    strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()),
                    THTTPHdrVal(REINTERPRET_CAST(TInt, iConnection)) );
        }
        else {
            // 使用内部连接
            connInfo.SetPropertyL(
                strP.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()),
                THTTPHdrVal(m_socketServ.Handle()));

            connInfo.SetPropertyL(
                strP.StringF(HTTP::EHttpSocketConnection, RHTTPSession::GetTable()),
                THTTPHdrVal(REINTERPRET_CAST(TInt, &(m_connection))));
        }

        if (m_isWap) {
            RStringF iPrxAddr = strP.OpenFStringL(KHttpProxy);
            CleanupClosePushL(iPrxAddr);

            THTTPHdrVal iPrxUsage(strP.StringF(HTTP::EUseProxy, RHTTPSession::GetTable()));
            connInfo.SetPropertyL(strP.StringF(HTTP::EProxyUsage, RHTTPSession::GetTable()), iPrxUsage);
            connInfo.SetPropertyL(strP.StringF(HTTP::EProxyAddress, RHTTPSession::GetTable()), iPrxAddr);
            CleanupStack::PopAndDestroy();
        }

        // set shutdown
        THTTPHdrVal immediateShutdown = strP.StringF(HTTP::ESocketShutdownImmediate, stringTable);
        connInfo.SetPropertyL(strP.StringF(HTTP::ESocketShutdownMode, stringTable), immediateShutdown);
        
//        // set pipelining
//        RStringF maxConnection = strP.StringF(HTTP::EMaxNumTransportHandlers, stringTable);
//        connInfo.SetPropertyL(maxConnection, THTTPHdrVal(KHttpMaxConnectionNum));
//
//        RStringF maxToPipeline = strP.StringF(HTTP::EMaxNumTransactionsToPipeline, stringTable);
//        connInfo.SetPropertyL(maxToPipeline, THTTPHdrVal(KHttpMaxTransactionNumPerConnection));

//        // Set optimal pipelining
//        RStringF strOptimalPipelineVal = strP.StringF(HTTP::EHttpEnableOptimalPipelining, RHTTPSession::GetTable());
//        connInfo.SetPropertyL(strP.StringF(HTTP::EHttpOptimalPipelining, RHTTPSession::GetTable()), strOptimalPipelineVal);

        // set HTTP receive Buffer Size property
//        RStringF receiveBuffSize = strP.StringF(HTTP::ERecvBufferSize, stringTable);
//        connInfo.SetPropertyL(receiveBuffSize, THTTPHdrVal(KHttpReceiveBuffSize));

//        // set HTTP batching enable
//        THTTPHdrVal batchingSupport(strP.StringF(HTTP::EEnableBatching, RHTTPSession::GetTable()));
//        connInfo.SetPropertyL(strP.StringF(HTTP::EHttpBatching, RHTTPSession::GetTable()), batchingSupport);

//        // set HTTP batching Buffer Size property
//        RStringF batchingBuffSize = strP.StringF(HTTP::EBatchingBufferSize, stringTable);
//        connInfo.SetPropertyL(batchingBuffSize, THTTPHdrVal(KHttpBatchingBuffSize));
        
        RHTTPFilterCollection filtColl = m_httpSession.FilterCollection();
        RStringF filterName = m_httpSession.StringPool().StringF(HTTP::ERedirect,RHTTPSession::GetTable());
        filtColl.RemoveFilter(filterName);
    }

    return EReady;
}

void CResourceManager::CloseHttpSession()
{
    if (m_sessionRunning) {
        m_httpSession.Close();
        m_sessionRunning = false;
    }
}

TInt CResourceManager::SetIap(TUint32 aIapId, TBool aIsWap)
{
    m_bUseOutNetSettting = false;

    m_socketServ.Connect();
    m_connection.Open(m_socketServ);

    m_iapId = aIapId;
    m_isWap = aIsWap;

    TInt err = KErrNone;

    // Now we have the iap Id. Use it to connect for the connection.
    // Create a connection preference variable.
    TCommDbConnPref connectPref;

    // setup preferences 
    connectPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    connectPref.SetDirection(ECommDbConnectionDirectionUnknown);
    connectPref.SetBearerSet(ECommDbBearerGPRS | ECommDbBearerWLAN | ECommDbBearerLAN);
    //Sets the CommDb ID of the IAP to use for this connection
    connectPref.SetIapId(aIapId);

    err = m_connection.Start(connectPref);

    if (err == KErrNone) {
        m_IAPSeted = true;
        m_IAPReady = true;
        ResendRequest();
    }

    return err;
}

void CResourceManager::SetAccessPoint(RConnection& aConnect, RSocketServ& aSocket,
    TUint32 aAccessPoint, const TDesC& aAcessName, const TDesC& aApnName, TBool aReady)
{
    m_bUseOutNetSettting = true;

    if (!aReady) {
        // 将所有等待请求置为失败
        MResConn* c = (MResConn*) sll_first(iConnList);
        while (c) {
            if (MResConn::EWaiting == c->iConnState) {
                CHttpConn* httpConn = static_cast<CHttpConn*> (c);
                if (httpConn)
                    httpConn->OnIapFailed();
            }
            c = (MResConn*) sll_next(iConnList);
        }
        return;
    }

    if (iConnection) {
        if (iConnection == &aConnect)
            return;

        CloseHttpSession();
    }

    iConnection = &aConnect;
    iSocketServ = &aSocket;
    m_iapId = aAccessPoint;

    if (aApnName == KCMCCCmwap || aApnName == KUniCmwap) {
        m_isWap = true;
    }

    if (iConnection && iSocketServ) {
        m_IAPSeted = true;
        if (!m_IAPReady) {
            m_IAPReady = true;
        }
        ResendRequest();
    }
}

void CResourceManager::ResetIap(void)
{
    m_IAPReady = false;
    CloseHttpSession();

    if (m_bUseOutNetSettting) {
        iConnection = NULL;
        iSocketServ = NULL;
    }
    else {
        m_connection.Close();
        m_socketServ.Close();
    }
}

void CResourceManager::OnHTTPIapNotReady(void* aKernel)
{
    ResetIap();
    if (aKernel)
        ((CNBrKernel*) aKernel)->RequestAP();
}

CProbe* CResourceManager::GetProbe()
{
    return l_resourceMananger->iProbe;
}

HBufC8* CResourceManager::GetUserAgent()
{
    return l_resourceMananger->iUserAgent;
}

void CResourceManager::Download(MResConn* conn, HBufC8* aUrl, HBufC8* aFileName,HBufC8* aCookie, TInt aSize)
{
    NBK_PlatformData* pfd = (NBK_PlatformData*) conn->iRequest->platform;
    CNBrKernel* nbk = (CNBrKernel*) pfd->nbk;
    HBufC16* name = NULL;

    if (aUrl == NULL)
        return;
    
    if (aFileName) {
        name = HBufC16::NewL(aFileName->Length());
        name->Des().Copy(*aFileName);
    }
    else
        name = CResourceManager::MakeFileName(*aUrl);
            
    nbk->Download(*aUrl, *name, KNullDesC8, aSize);
    
    if (name) {
        delete name;
    }
}

HBufC16* CResourceManager::MakeFileName(const TDesC8& aUrl)
{
    HBufC16* fileName = NULL;
    if (aUrl.Length()) {
        char* p = (char*) aUrl.Ptr();
        char* tooFar = p + aUrl.Length();
        if(*(tooFar-1) == '/')
            tooFar--;
        char* q = tooFar - 1;
        while (q > p && *q != '/')
            q--;
        if (*q == '/' && *(q + 1) != '/' && q < tooFar - 1) {
            q++;
            p = q;
            while (*p != '?' && p < tooFar)
                p++;
            int l;
            wchr* nameu16 = uni_utf8_to_utf16_str((uint8*) q, p - q, &l);
            fileName = HBufC16::NewL(l);
            fileName->Des().Copy(nameu16, l);
            NBK_free(nameu16);
        }
    }
    return fileName;
}

void CResourceManager::OnWaiting(MResConn* conn)
{
    NBK_PlatformData* pfd = (NBK_PlatformData*) conn->iRequest->platform;
    CNBrKernel* nbk = (CNBrKernel*) pfd->nbk;

    CNBrKernel::OnNbkEvent(NBK_EVENT_WAITING, nbk, NULL);
}

void CResourceManager::SetCachePath(const TDesC& aPath)
{
    if (iCachePath)
        delete iCachePath;

    iCachePath = aPath.Alloc();
}

TPtr CResourceManager::GetCachePath()
{
    if (iCachePath == NULL) {
        TPtrC p(KCachePath);
        iCachePath = p.Alloc();
    }

    return iCachePath->Des();
}

void CResourceManager::GetMsgDigestByMd5L(TDes8 &aDest, const TDesC8 &aSrc)
{
    _LIT8( KDigestFormat, "%02x" );
    aDest.Zero();
    CMD5 *md5 = CMD5::NewL();
    CleanupStack::PushL(md5);
    TPtrC8 ptrHash = md5->Hash(aSrc);
    for (TInt i = 0; i < ptrHash.Length(); i++) {
        aDest.AppendFormat(KDigestFormat, ptrHash[i]);
    }
    CleanupStack::PopAndDestroy(md5);
}

void CResourceManager::GetMd5(const TDesC8& aString, TUint8* aMd5)
{
    CMD5 *md5 = CMD5::NewL();
    CleanupStack::PushL(md5);
    TPtrC8 ptrHash = md5->Hash(aString);
    const TUint8* p = ptrHash.Ptr();
    Mem::Copy(aMd5, p, ptrHash.Length());
    CleanupStack::PopAndDestroy(md5);
}

CFEPControl* CResourceManager::GetFEPControl(void)
{
    if(!iSysControl)
    {
        iSysControl = CFEPControl::NewL();
        iSysControl->DeActive();   
    }
    return iSysControl;
}

void CResourceManager::CleanCachedPage(nid aPageId)
{
    int len = GetCachePath().Length();
    HBufC* cache = HBufC::New(len + 64);
    TPtr p = cache->Des();
    TPtr path = GetCachePath();
    p.Format(KCachePathFormat, &path, aPageId);
    
    _LIT(KWildName, "*.*");
    RFs& fs = CCoeEnv::Static()->FsSession();
    TFindFile file_finder(fs);
    CDir* file_list;
    TInt err = file_finder.FindWildByDir(KWildName, p, file_list);
    while (err == KErrNone) {
        for (TInt i = 0; i < file_list->Count(); i++) {
            if ((*file_list)[i].IsDir())
                continue;

            TParse fullentry;
            fullentry.Set((*file_list)[i].iName, &file_finder.File(), NULL);
            if (fullentry.ExtPresent()) {
                err = fs.Delete(fullentry.FullName());
            }
            else {
                TFileName filename;
                filename.Append(fullentry.DriveAndPath());
                filename.Append(fullentry.Name());
                err = fs.Delete(filename);
            }
        }
        delete file_list;
        err = file_finder.FindWild(file_list);
    }

    err = fs.RmDir(p);
    delete cache;
}

void CResourceManager::DumpConn(MResConn* aConn)
{
    NRequest* req = aConn->iRequest;

    iProbe->OutputChar("page", -1);
    iProbe->OutputInt(aConn->PageId());
    iProbe->OutputChar(req->url, -1);
    iProbe->OutputReturn();
}

void CResourceManager::DumpConnList()
{
    iProbe->OutputChar("==================== Conn List ====================", -1);
    iProbe->OutputReturn();

    MResConn* c = (MResConn*) sll_first(iConnList);
    while (c) {
        DumpConn(c);
        c = (MResConn*) sll_next(iConnList);
    }

    iProbe->OutputChar("---------------------------------------------------", -1);
    iProbe->OutputReturn();
}

CDecodeThread* CResourceManager::GetDecodeThread(void)
{
    if (!iDecodeThread)
        iDecodeThread = CDecodeThread::NewL();

    return iDecodeThread;
}

void CResourceManager::UseNightTheme(TBool aUse)
{
    int adId, flashId;
    
    if (aUse) {
        adId = EMbmNbk_coreAd_dark;
        flashId = EMbmNbk_coreFlash_dark;
    }
    else {
        adId = EMbmNbk_coreAd;
        flashId = EMbmNbk_coreFlash;
    }
}

void CResourceManager::ClearCookies()
{
}
