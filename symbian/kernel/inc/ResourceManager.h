/*
 ============================================================================
 Name		: ResourceManager.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CResourceManager declaration
 ============================================================================
 */

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

// INCLUDES
#include "../../../stdc/inc/config.h"
#include "../../../stdc/loader/loader.h"
#include "../../../stdc/loader/cachemgr.h"
#include "../../../stdc/loader/cookiemgr.h"
#include "../../../stdc/tools/slist.h"
#include <e32std.h>
#include <e32base.h>
#include <es_sock.h>
#include <rhttpsession.h>
#include <Etel3rdParty.h>
#include "MResConn.h"
#include "ImageImpl.h"

_LIT(KCachePathFormat,      "%Scp%05d\\");
_LIT(KCacheDocFormat,       "%Scp%05d\\doc");
_LIT(KCacheUriFormat,       "%Scp%05d\\uri");

_LIT(KHtmlSuffix,           ".htm");
_LIT(KGzipSuffix,           ".gz");
_LIT(KGifSuffix,            ".gif");
_LIT(KJpegSuffix,           ".jpg");
_LIT(KPngSuffix,            ".png");
_LIT(KCszSuffix,            ".csz");
_LIT(KCssSuffix,            ".css");
_LIT(KWbxmlSuffix,          ".wb");

// CLASS DECLARATION

class CProbe;
class CFEPControl;
class CDecodeThread;

/**
 *  CResourceManager
 * 
 */
class CResourceManager : public CBase
{
public:
    ~CResourceManager();
    static CResourceManager* NewL();
    static CResourceManager* NewLC();
    static void Destroy();
    static CResourceManager* GetPtr();
    
public:
    void AddConn(MResConn* conn);
    void StopAll(nid aPageId);
    void CleanCachedPage(nid aPageId);
    
    NCookieMgr* GetCookieManager() const { return iCookieMgr; }
    void ClearCookies();
    
    NCacheMgr* GetCacheMgr() { return iCacheMgr; }
    
    void SetCachePath(const TDesC& aPath);
    TPtr GetCachePath();
    
    static CProbe* GetProbe();
    static HBufC8* GetUserAgent();
    
    static void OnWaiting(MResConn* conn);
    static void OnReceiveHeader(MResConn* conn, NRespHeader* header);
    static void OnReceiveData(MResConn* conn, char* data, int length, int comprLen);
    static void OnComplete(MResConn* conn);
    static void OnCancel(MResConn* conn);
    static void OnError(MResConn* conn, int err);
    
    static void Download(MResConn* conn, HBufC8* aUrl, HBufC8* aFileName, HBufC8* aCookie, TInt aSize);
    
    static TInt OnCleanUnusedConn(TAny* ptr);
    
    static HBufC16* MakeFileName(const TDesC8& aUrl);
    
    TBool IsWapIAP(void) { return m_isWap; }
    
    void UseNightTheme(TBool aUse = ETrue);
    
    CDecodeThread* GetDecodeThread(void);
    
public:

    typedef enum _TSessionState {
        EReady,
        ENotReady,
        ENotSetIAP,
    } EHttpSessionState;

    EHttpSessionState OpenHttpSessionIfNeededL(void* aKernel);
    void CloseHttpSession();
    RHTTPSession& httpSession() { return m_httpSession; }
    TInt SetIap(TUint32 aIapId, TBool aIsWap);
    void SetAccessPoint(RConnection& aConnect, RSocketServ& aSocket, TUint32 aAccessPoint,
        const TDesC& aAcessName, const TDesC& aApnName,TBool aReady);
    void ResetIap(void);
    void OnHTTPIapNotReady(void* aKernel);
    
public:
    CProbe* iProbe;
    
    /*
     * 调用样例：
     * 
        TBuf8<32> md5str;
        TBuf<32> md5strU;
        CResourceManager::GetMsgDigestByMd5L(md5str, url);
        md5strU.Copy(md5str);
    */

    static void GetMsgDigestByMd5L(TDes8 &aDest, const TDesC8 &aSrc);
    static void GetMd5(const TDesC8& aString, TUint8* aMd5);
    
    CFEPControl* GetFEPControl(void);
    
private:
    CResourceManager();
    void ConstructL();
    
    void ResendRequest();
    
    void GenPhoneID();
    
private:
    CPeriodic* iTimer;
    NSList* iConnList;

    void StartClean();
    void CleanUnusedConn();
    void CleanAllConn(nid aPageId);
    
    void DumpConn(MResConn* aConn);
    void DumpConnList();
    
private:
    TUint32 m_iapId;
    
    bool m_isWap;
    bool m_IAPSeted;
    bool m_sessionRunning;
    bool m_bUseOutNetSettting;
    bool m_IAPReady;
    
    RConnection* iConnection; // from shell
    RSocketServ* iSocketServ;
    RConnection m_connection; // owned
    RSocketServ m_socketServ;
    RHTTPSession m_httpSession;
    
    NCookieMgr* iCookieMgr;
    NCacheMgr* iCacheMgr;

    CTelephony::TPhoneIdV1 idV1;

    HBufC8* iUserAgent; // 标准UA
    HBufC8* iImei; // 内部惟一标识
    
    HBufC*  iCachePath;
    
    int   iFileIdx;
    
    CFEPControl*   iSysControl;
    
    CDecodeThread* iDecodeThread; // sync decoder thread
};

#endif // RESOURCEMANAGER_H
