/*
 ============================================================================
 Name		: NBrKernel.cpp
 Author	  : wuyulun
 Copyright   : 2010,2011 (C) MIC
 Description : CNBrKernel DLL source
 ============================================================================
 */

#include "../../../stdc/inc/config.h"
#include "../../../stdc/inc/nbk_settings.h"
#include "../../../stdc/inc/nbk_cbDef.h"
#include "../../../stdc/inc/nbk_callback.h"
#include "../../../stdc/dom/history.h"
#include "../../../stdc/dom/node.h"
#include "../../../stdc/dom/page.h"
#include "../../../stdc/dom/document.h"
#include "../../../stdc/dom/view.h"
#include "../../../stdc/dom/event.h"
#include "../../../stdc/render/renderNode.h"
#include "../../../stdc/loader/loader.h"
#include "../../../stdc/loader/url.h"
#include "../../../stdc/loader/upcmd.h"
#include "../../../stdc/tools/str.h"
#include "../../../stdc/tools/slist.h"
#include "../../../stdc/editor/textSel.h"

#include <utf.h>
#include <e32cmn.h>
#include <math.h>
#include "NBrKernel.h"
#include "NBrKernel.pan"

#include "NbkGdi.h"
#include "ResourceManager.h"
#include "TimerManager.h"
#include "Probe.h"
#include "ImageManager.h"
#include "nbk_core.mbg"
#include "FEPHandler.h"

#ifdef __NBK_SELF_TESTING__
#include "Monkey.h"
#endif

#define DEBUG_EVENT_QUEUE   0
#define DEBUG_SCROLL_VIEW   0

#define NBK_MAX_EVENT_QUEUE     64
#define EVTQ_INC(i) ((i < NBK_MAX_EVENT_QUEUE - 1) ? i+1 : 0)
#define EVTQ_DEC(i) ((i > 0) ? i-1 : NBK_MAX_EVENT_QUEUE - 1)

#define MOVE_DISTANCE_BY_ARROW  0.5
#define MOVE_DISTANCE_BY_SCREEN 0.8

#define ARROW_MOVE_STEP 12
#define ARROW_STOP      2

#define PEN_MOVE_MIN    24

#define FIXED_SCREEN_WIDTH  0
#define SCREEN_WIDTH        240

#define GET_NBK_PAGE(history)   (history_curr((NHistory*)history))

static int l_refCounter = 0;

#if DEBUG_EVENT_QUEUE
static char* l_evtDesc[NBK_EVENT_LAST] = {
    "none",
    "new document",
    "history document",
    "do ia",
    "waiting",
    "got response",
    "new document begin",
    "got data",
    "update (size changed)",
    "update pics",
    "layout finish",
    "image downloading",
    "paint control",
    "enter main body",
    "quit main body",
    "quit main body after click",
    "loading error page",
    "reposition",
    "got inc-data",
    "download attachment",
    "text select finish",
    "debug"
};
#endif

// 接收来自内核的事件通知
int CNBrKernel::OnNbkEvent(int id, void* user, void* info)
{
    CNBrKernel* self = (CNBrKernel*)user;

    // 调试输出
    if (id == NBK_EVENT_DEBUG1) {
        self->iProbe->Output(info);
        return 0;
    }
    // 附件下载通知
    if (id == NBK_EVENT_DOWNLOAD_FILE) {
        NBK_Event_DownloadFile* evt = (NBK_Event_DownloadFile*)info;
        TInt len = nbk_strlen(evt->url);
        TPtrC8 url((const TUint8*)evt->url, len);
        HBufC16* name = NULL;

        if (evt->fileName) {
            TInt len = nbk_strlen(evt->fileName);
            TPtrC8 fileName((const TUint8*)evt->fileName, len);
            name = HBufC16::NewL(len);
            name->Des().Copy(fileName);
        }
        else {
            name = CResourceManager::MakeFileName(url);
        }
        
        if (name) {
            self->Download(url, *name, KNullDesC8, evt->size);
            delete name;
        }
        return 0;
    }

    // 以下事件为异步处理
    
    TNbkEvent* nevt;
    TNbkEvent* eq = self->iEvtQueue;

    if (   id == NBK_EVENT_DOWNLOAD_IMAGE
        || id == NBK_EVENT_UPDATE_PIC
        || id == NBK_EVENT_GOT_DATA
        || id == NBK_EVENT_REPOSITION ) {
        
        // 查找未处理事件
        int pos = EVTQ_DEC(self->iEvtAdd);
        nevt = NULL;
        while (eq[pos].eventId && pos != self->iEvtAdd) {
            if (eq[pos].eventId == id) {
                nevt = &eq[pos];
                break;
            }
            pos = EVTQ_DEC(pos);
        }
        
        if (nevt) {
            if (id == NBK_EVENT_DOWNLOAD_IMAGE) {
                NBK_Event_GotImage* evt = (NBK_Event_GotImage*)info;
                nevt->imgCur = evt->curr;
                nevt->imgTotal = evt->total;
                nevt->datReceived = evt->receivedSize;
                nevt->datTotal = evt->totalSize;
            }
            else if (id == NBK_EVENT_GOT_DATA) {
                NBK_Event_GotResponse* evt = (NBK_Event_GotResponse*)info;
                nevt->datReceived = evt->received;
                nevt->datTotal = evt->total;
            }
            else if (id == NBK_EVENT_REPOSITION) {
                NBK_Event_RePosition* evt = (NBK_Event_RePosition*)info;
                nevt->x = evt->x;
                nevt->y = evt->y;
                nevt->zoom = evt->zoom;
            }

#if DEBUG_EVENT_QUEUE > 1
            self->iProbe->OutputChar("-x- Event", -1);
            self->iProbe->OutputChar(l_evtDesc[nevt->eventId], -1);
            self->iProbe->OutputReturn();
#endif
            return 0;
        }
    }

    // 将事件加入事件队列
    nevt = &eq[self->iEvtAdd];
    self->iEvtAdd = EVTQ_INC(self->iEvtAdd);
    NBK_memset(nevt, 0, sizeof(TNbkEvent));
    nevt->eventId = id;

#if DEBUG_EVENT_QUEUE > 1
    char counter[16];
    sprintf(counter, "(%2d / %2d)", self->iEvtUsed, self->iEvtAdd);
    self->iProbe->OutputChar("--> Event", -1);
    self->iProbe->OutputChar(counter, -1);
    self->iProbe->OutputChar(l_evtDesc[nevt->eventId], -1);
    self->iProbe->OutputReturn();
#endif

    switch (id) {
    
    // 准备发出新文档请求
    case NBK_EVENT_NEW_DOC:
    {
        self->iArrowPos.SetXY(ARROW_STOP, ARROW_STOP);
        self->iUseMinZoom = ETrue;
        break;
    }
    case NBK_EVENT_NEW_DOC_BEGIN:
        self->iGdi->GetFontMgr()->Reset(); // 清除字体表，同步调用
        break;
        
    case NBK_EVENT_HISTORY_DOC:
    {
        self->iUseMinZoom = EFalse;
        break;
    }
    
    // 接收到数据
    case NBK_EVENT_GOT_DATA:
    {
        NBK_Event_GotResponse* evt = (NBK_Event_GotResponse*)info;
        nevt->datReceived = evt->received;
        nevt->datTotal = evt->total;
        break;
    }
    
    // 正在下载图片
    case NBK_EVENT_DOWNLOAD_IMAGE:
    {
        NBK_Event_GotImage* evt = (NBK_Event_GotImage*)info;
        nevt->imgCur = evt->curr;
        nevt->imgTotal = evt->total;
        nevt->datReceived = evt->receivedSize;
        nevt->datTotal = evt->totalSize;
        break;
    }

    // 排版完成（返回上次查看本页的位置）
    case NBK_EVENT_LAYOUT_FINISH:
    // 文档重定位
    case NBK_EVENT_REPOSITION:
    {
        NBK_Event_RePosition* evt = (NBK_Event_RePosition*)info;
        nevt->x = evt->x;
        nevt->y = evt->y;
        nevt->zoom = evt->zoom;
        break;
    }

    default:
        break;
    }

    // 调度事件
    self->iTimer->Cancel();
    self->iTimer->Start(10, 5000000, TCallBack(&OnTimer, user));

    return 0;
}

void CNBrKernel::HandleNbkEvent()
{
    TNbkEvent* nevt;

    nevt = &iEvtQueue[iEvtUsed];
    if (nevt->eventId == 0)
        return;

#if DEBUG_EVENT_QUEUE
    char counter[16];
    BOOL flush = EFalse;
    sprintf(counter, "(%2d / %2d)", iEvtUsed, iEvtAdd);
    iProbe->OutputChar("*** Event", -1);
    iProbe->OutputChar(counter, -1);
    iProbe->OutputChar(l_evtDesc[nevt->eventId], -1);
    if (nevt->eventId == NBK_EVENT_NEW_DOC_BEGIN) {
        iProbe->OutputChar("==========>>>", -1);
        flush = ETrue;
    }
    else if (nevt->eventId == NBK_EVENT_GOT_DATA) {
        iProbe->OutputInt(nevt->datReceived);
        iProbe->OutputInt(nevt->datTotal);
    }
    else if (nevt->eventId == NBK_EVENT_DOWNLOAD_IMAGE) {
        iProbe->OutputInt(nevt->imgCur);
        iProbe->OutputInt(nevt->imgTotal);
    }
    iProbe->OutputReturn();
    if (flush)
        iProbe->Flush();
#endif

    iEventId = nevt->eventId;
    switch (nevt->eventId) {

    // 准备发出新文档请求
    case NBK_EVENT_NEW_DOC:
    {
        iShellInterface->OnNewDocument();
        if (iAuto) {
            _LIT(KText, "send request...");
            DrawText(KText);
            iShellInterface->NbkRedraw();
        }
        break;
    }

    // 打开历史页面
    case NBK_EVENT_HISTORY_DOC:
    {
        iShellInterface->OnHistoryDocument();
        if (iAuto) {
            _LIT(KText, "open history...");
            DrawText(KText);
            iShellInterface->NbkRedraw();
        }
        break;
    }

    // Post 数据已发出，等待响应
    case NBK_EVENT_WAITING:
    {
        iShellInterface->OnWaiting();
        if (iAuto) {
            _LIT(KText, "waiting...");
            DrawText(KText);
            iShellInterface->NbkRedraw();
        }
        break;
    }

    // 接收到数据，开始建立文档
    case NBK_EVENT_NEW_DOC_BEGIN:
    {
        Clear();
        iViewRect.l = iViewRect.t = iViewRect.r = iViewRect.b = 0;
        iShellInterface->OnNewDocumentBegin();
        break;
    }

    // 收到服务器回应
    case NBK_EVENT_GOT_RESPONSE:
    {
        iShellInterface->OnGotResponse();
        if (iAuto) {
            _LIT(KText, "got response...");
            DrawText(KText);
            iShellInterface->NbkRedraw();
        }
        break;
    }

    // 接收到数据
    case NBK_EVENT_GOT_DATA:
    {
        iShellInterface->OnReceivedData(nevt->datReceived, nevt->datTotal);
        if (iAuto) {
            TBuf<48> mess;
            mess.Format(_L("receiving %d / %d ..."), nevt->datReceived, nevt->datTotal);
            DrawText(mess);
            iShellInterface->NbkRedraw();
        }
        break;
    }

    // 更新视图，文档尺寸有改变
    case NBK_EVENT_UPDATE:
    {
        CalcMinZoomFactor();
        iShellInterface->OnViewUpdate(ETrue);
        if (iAuto) {
            UpdateScreen();
        }
        break;
    }

    // 更新视图，文档尺寸无改变
    case NBK_EVENT_UPDATE_PIC:
    {
        iShellInterface->OnViewUpdate(EFalse);
        if (iAuto) {
            UpdateScreen();
        }
        break;
    }

    // 文档排版完成
    case NBK_EVENT_LAYOUT_FINISH:
    {
        CalcMinZoomFactor();
        float zoom = ifCurZoomFactor;
        if (ifMinZoomfactor > 1.0)
            zoom /= ifMinZoomfactor;
        iShellInterface->OnLayoutFinish(nevt->x, nevt->y, zoom);
        if (iAuto) {
            iLastPos.iX = nevt->x;
            iLastPos.iY = nevt->y;
            UpdateScreen();
        }
        break;
    }

    // 正在下载图片
    case NBK_EVENT_DOWNLOAD_IMAGE:
    {
        iShellInterface->OnReceivedImage(nevt->imgCur, nevt->imgTotal);
        if (iAuto) {
            if (nevt->imgCur < nevt->imgTotal) {
                iMessage.Format(_L("download images %d / %d ..."), nevt->imgCur, nevt->imgTotal);
                DrawText(iMessage);
                iShellInterface->NbkRedraw();
            }
            else {
                iMessage = _L("");
                UpdateScreen();
            }
        }

        break;
    }

    // 绘制控件，文档尺寸无改变
    case NBK_EVENT_PAINT_CTRL:
    {
        if (iAuto)
            UpdateScreen();
        iShellInterface->OnViewUpdate(EFalse);
        break;
    }

    // 通知即将载入错误页
    case NBK_EVENT_LOADING_ERROR_PAGE:
    {
        iShellInterface->OnLoadingErrorPage();
        break;
    }

    // 文档重定位
    case NBK_EVENT_REPOSITION:
    {
        float zoom = nevt->zoom;
        if (ifMinZoomfactor > 1.0)
            zoom /= ifMinZoomfactor;
        iShellInterface->OnRePosition(nevt->x, nevt->y, zoom);

        if (iAuto) {
            SetCurZoom(nevt->zoom);
            RePos(nevt->x, nevt->y);
            UpdateScreen();
        }
        break;
    }
    
    case NBK_EVENT_TEXTSEL_FINISH:
    {
        iShellInterface->OnTextselFinish();
        break;
    }
    
    } // switch

#ifdef __NBK_SELF_TESTING__
    if (iMonkeyTester)
    iMonkeyTester->OnNbkEvent(*nevt);
#endif

    NBK_memset(nevt, 0, sizeof(TNbkEvent));
    iEvtUsed = EVTQ_INC(iEvtUsed);
    if (iEvtQueue[iEvtUsed].eventId) {
        // 存在未处理事件
        iTimer->Cancel();
        iTimer->Start(10, 5000000, TCallBack(&OnTimer, this));
    }
}

TInt CNBrKernel::OnTimer(TAny* ptr)
{
    CNBrKernel* self = (CNBrKernel*) ptr;
    self->iTimer->Cancel();
    self->HandleNbkEvent();
    return 0;
}

EXPORT_C CNBrKernel* CNBrKernel::NewLC(MNbrKernelInterface* interface)
{
    CNBrKernel* self = new (ELeave) CNBrKernel(interface);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

EXPORT_C CNBrKernel* CNBrKernel::NewL(MNbrKernelInterface* interface)
{
    CNBrKernel* self = CNBrKernel::NewLC(interface);
    CleanupStack::Pop(self);
    return self;
}

CNBrKernel::CNBrKernel(MNbrKernelInterface* interface)
{
    iAuto = ETrue;
    iShellInterface = interface;
    iCounter = 0;
    iDenyEvent = EFalse;

    iDbcInterval = 0;

    iTouchScreen = EFalse;
    iEditPaused = EFalse;
    iFepShrinkSize.SetSize(1, 2);
}

void CNBrKernel::ConstructL()
{
    iPageSettings = NBK_malloc0(sizeof(NSettings));
    NSettings* settings = (NSettings*)iPageSettings;
    settings->allowImage = 1;
    settings->initMode = NEREV_STANDARD;
    settings->mode = NEREV_TF; // 中转浏览下优先选择的浏览模式
    settings->mainFontSize = 0; // 采用默认字体大小
    settings->fontLevel = NEFNTLV_MIDDLE;
    settings->selfAdaptionLayout = 0; // 自适应模式
    settings->clickToLoadImage = 0;
    
    iNightTheme = NBK_malloc0(sizeof(NNightTheme));
    NNightTheme* theme = (NNightTheme*)iNightTheme;
    color_set(&theme->text, 0x73, 0x82, 0x95, 0xff);
    color_set(&theme->link, 0x28, 0x5a, 0x8c, 0xff);
    color_set(&theme->background, 0x0a, 0x0a, 0x0a, 0xff);
//    settings->night = theme;
    
    l_refCounter++;

    //iFepYOffset = 50;

    // 事件队列
    iEvtQueue = (TNbkEvent*)NBK_malloc0(sizeof(TNbkEvent) * NBK_MAX_EVENT_QUEUE);
    iEvtAdd = iEvtUsed = 0;

//    // 诊断，输出功能
//    iProbe = new CProbe();

    // 初始化资源管理器
    CResourceManager* resMgr = CResourceManager::GetPtr();
    iProbe = resMgr->iProbe;
    
    // 初始化图像管理器
    iImageManager = CImageManager::NewL();
    iImageManager->iProbe = iProbe;
    iImageManager->iNbk = this;

    // create GDI
    iGdi = new CNbkGdi(this);
    iGdi->iProbe = iProbe;

    iPlatform.probe = iProbe;
    iPlatform.imageManager = iImageManager;
    iPlatform.gdi = iGdi;
    iPlatform.nbk = this;

    // create nbk page
    NHistory* history = history_create(&iPlatform);
    iHistory = history;
    history_setSettings(history, (NSettings*)iPageSettings);
    history_enablePic(history, ((NSettings*)iPageSettings)->allowImage);
    NBK_Page* page = GET_NBK_PAGE(iHistory);
    page_setEventNotify(page, (NBK_CALLBACK)OnNbkEvent, this);

    iInfoFont = iGdi->GetFont(14, ETrue, EFalse);

    // 载入光标位图
    {
        TFileName drive;
        TFileName fileName;
        TParse parse;

        Dll::FileName(drive);
        parse.Set(drive, NULL, NULL);

        fileName += parse.Drive();
        fileName += _L("\\resource\\apps\\NBrKernel.mbm");

        iBmpArrow = new (ELeave) CFbsBitmap;
        iBmpArrow->Load(fileName, EMbmNbk_coreArrowcursor);
        iBmpArrowMask = new (ELeave) CFbsBitmap;
        iBmpArrowMask->Load(fileName, EMbmNbk_coreArrowcursormask);
    }

    iTimer = CPeriodic::NewL(CActive::EPriorityHigh);

    iNodeState = MNbkCtrlPaintInterface::ENormal;
}

EXPORT_C CNBrKernel::~CNBrKernel()
{
#ifdef __NBK_SELF_TESTING__
    ResetMonkey();
#endif
    
    m_shortcutTextLst.ResetAndDestroy();
    delete iScreenGc;
    delete iScreenDev;

    iTimer->Cancel();
    delete iTimer;

    NBK_History* h = (NBK_History*) iHistory;
    history_delete(&h);

    delete iGdi;
    
    l_refCounter--;
    if (l_refCounter == 0) {        
        CTimerManager::Destroy();
        CResourceManager::Destroy();
    }
    
    delete iImageManager;

    if (iBmpArrow) {
        delete iBmpArrow;
        delete iBmpArrowMask;
    }
    
    if (iFepHandler) {
        delete iFepHandler;
        iFepHandler = 0;
    }

    NBK_free(iEvtQueue);
    NBK_free(iPageSettings);
    NBK_free(iNightTheme);
    iEvtQueue = NULL;
//    delete iProbe;
    
    CloseSTDLIB();
}

EXPORT_C TVersion CNBrKernel::Version() const
{
    // Version number of example API
    const TInt KMajor = 1;
    const TInt KMinor = 0;
    const TInt KBuild = 1;
    return TVersion(KMajor, KMinor, KBuild);
}

EXPORT_C void CNBrKernel::SetScreen(CFbsBitmap* aScreen, TBool aFixedScreen)
{
    if (iScreenGc) {
        delete iScreenGc;
        delete iScreenDev;
    }

    iScreen = aScreen;
    iScreenDev = CFbsBitmapDevice::NewL(iScreen);
    CleanupStack::PushL(iScreenDev);
    User::LeaveIfError(iScreenDev->CreateContext(iScreenGc));
    CleanupStack::Pop();

    if (iAuto) {
        iScreenGc->SetBrushColor(TRgb(0, 0, 0, 128));
        iScreenGc->Clear();
    }

    SetPageWidth();
    
    ((NSettings*)iPageSettings)->fixedScreen = (aFixedScreen) ? 1 : 0;
}

void CNBrKernel::DrawCounter()
{
    if (iScreenGc == NULL)
        return;

    TBuf<8> num;
    num.Format(_L("%d"), ++iCounter);

    const CFont* font = CCoeEnv::Static()->NormalFont();
    iScreenGc->Clear();
    iScreenGc->SetPenColor(KRgbRed);
    iScreenGc->UseFont(font);
    iScreenGc->DrawText(num, TPoint(0, font->AscentInPixels()));
    iScreenGc->DiscardFont();
}

void CNBrKernel::DrawText(const TDesC& aText)
{
    if (iScreenGc) {
        int h = iGdi->GetFontHeight(iInfoFont);
        TSize s = iScreen->SizeInPixels();
        TRect r(0, 0, 0, 0);
        r.iTl.iY = s.iHeight - h;
        r.SetHeight(h);
        r.SetWidth(s.iWidth - s.iWidth / 10);
        iScreenGc->SetBrushColor(KRgbBlue);
        iScreenGc->Clear(r);
        iScreenGc->SetPenColor(KRgbYellow);
        iScreenGc->UseFont(iGdi->GetFont(iInfoFont));
        iScreenGc->DrawText(aText, TPoint(0, s.iHeight - h + iGdi->GetFontAscent(iInfoFont)));
        iScreenGc->DiscardFont();
        iScreenGc->SetBrushColor(KRgbWhite);
    }
}

void CNBrKernel::DrawScreen()
{
    if (iScreenGc == NULL)
        return;

    TRect r(iViewRect.l, iViewRect.t, iViewRect.r, iViewRect.b);
    DrawPage(r);

    iLastPos.SetXY(iViewRect.l, iViewRect.t);
}

void CNBrKernel::UpdateScreen()
{
    if (   iEventId != NBK_EVENT_UPDATE_PIC
        && iEventId != NBK_EVENT_PAINT_CTRL
        && iEventId != NBK_EVENT_REPOSITION )
        UpdateViewRect();

    if (iEventId == NBK_EVENT_PAINT_CTRL) {
        // 绘制单个控件，当文档尺寸小于屏幕尺寸时，绘图区域扩大到屏幕尺寸
        NRect r = *(NRect*)&iViewRect;
//        TSize lcd = iScreen->SizeInPixels();
//        r.r = (r.r < lcd.iWidth) ? lcd.iWidth : r.r;
//        r.b = (r.b < lcd.iHeight) ? lcd.iHeight : r.b;
        page_paintControl(GET_NBK_PAGE(iHistory), &r);
    }
    else
        DrawScreen();

    DrawArrow();

    if (iEventId == NBK_EVENT_LAYOUT_FINISH) {
        _LIT(KText, "Layout Finish");
        DrawText(KText);
    }
    else if (iMessage.Length())
        DrawText(iMessage);

    iShellInterface->NbkRedraw();
}

void CNBrKernel::RefreshScreen()
{
    DrawScreen();
    DrawArrow();
    iShellInterface->NbkRedraw();
}

void CNBrKernel::DrawArrow()
{
    if (!iAuto || iTouchScreen)
        return;

    if (!i4Ways)
        return;

    TPoint pt(iArrowPos);
    TRect dst(pt, iBmpArrow->SizeInPixels());
    TRect src(TPoint(0, 0), iBmpArrow->SizeInPixels());

    iScreenGc->DrawBitmapMasked(dst, iBmpArrow, src, iBmpArrowMask, ETrue);
}

EXPORT_C void CNBrKernel::LoadData(const TDesC8& aUrl, const HBufC8* aData)
{
    if (GET_NBK_PAGE(iHistory) == NULL)
        return;

    Clear();

    char* url = (char*) NBK_malloc0(aUrl.Length() + 1);
    nbk_strncpy(url, (char*) aUrl.Ptr(), aUrl.Length());
    page_loadData(GET_NBK_PAGE(iHistory), url, (const char*) aData->Ptr(), aData->Length());
    NBK_free(url);
}

EXPORT_C void CNBrKernel::LoadUrl(const TDesC8& aUrl)
{
    if (GET_NBK_PAGE(iHistory) == NULL)
        return;

    Clear();
    char* url = (char*) NBK_malloc0(aUrl.Length() + 1);
    nbk_strncpy(url, (char*) aUrl.Ptr(), aUrl.Length());
    page_loadUrl(GET_NBK_PAGE(iHistory), url);
    NBK_free(url);
}

// 文档尺寸改变，更新可视区域
void CNBrKernel::UpdateViewRect()
{
    TSize lcd = iScreen->SizeInPixels();
    NBK_Page* p = GET_NBK_PAGE(iHistory);
    coord w = page_width(p);
    coord h = page_height(p);

    iViewRect.l = 0;
    iViewRect.r = Min(w, lcd.iWidth);
    iViewRect.t = 0;
    iViewRect.b = Min(h, lcd.iHeight);

    if (iLastPos.iX) {
        coord vw = iViewRect.r - iViewRect.l;
        if (w > lcd.iWidth && iViewRect.l + iLastPos.iX < w) {
            iViewRect.l += iLastPos.iX;
            if (iViewRect.l + vw > w)
                iViewRect.l = w - vw;
            iViewRect.r = iViewRect.l + vw;
        }
    }
    if (iLastPos.iY) {
        coord vh = iViewRect.b - iViewRect.t;
        if (h > lcd.iHeight && iViewRect.t + iLastPos.iY < h) {
            iViewRect.t += iLastPos.iY;
            if (iViewRect.t + vh > h)
                iViewRect.t = h - vh;
            iViewRect.b = iViewRect.t + vh;
        }
    }
}

void CNBrKernel::CalcMinZoomFactor()
{
    TSize lcd = iScreen->SizeInPixels();
    NHistory* history = (NHistory*) iHistory;
    NBK_Page* p = history_curr(history);
    nid docType = doc_getType(p->doc);

    ifMinZoomfactor = KNbkDefaultZoomLevel;
    ifCurZoomFactor = history_getZoom(history);
    float fa = view_getMinZoom(p->view);
    iViewXOffset = 0;

    if (docType == NEDOC_FULL) {
        ifMinZoomfactor = fa;
    }
    else if (docType == NEDOC_NARROW) {
        coord w = page_width(p);
        if (w < lcd.iWidth)
            iViewXOffset = (lcd.iWidth - w) >> 1;
    }
    else if (docType != NEDOC_MAINBODY) {
        ifMinZoomfactor = fa;
        if (ifCurZoomFactor < ifMinZoomfactor || fabs(ifCurZoomFactor - ifMinZoomfactor) < NFLOAT_PRECISION) {
            history_setZoom(history, ifMinZoomfactor);
            coord w = page_width(p);
            if (w < lcd.iWidth)
                iViewXOffset = (lcd.iWidth - w) >> 1;
        }
    }

    TPoint pt = iDrawOffset;
    pt.iX += iViewXOffset;
    iGdi->SetDrawOffset(pt);

    // 新打开页面，记录适屏缩放因子
    if (iUseMinZoom) {
        history_setZoom(history, ifMinZoomfactor);
        ifCurZoomFactor = ifMinZoomfactor;
    }
}

void CNBrKernel::AdjustViewRect(TNbkRect* rect, TBool aDirDown)
{
    TSize lcd = iScreen->SizeInPixels();
    NBK_Page* p = GET_NBK_PAGE(iHistory);
    coord w = page_width(p);
    coord h = page_height(p);

    if (lcd.iHeight >= h)
        return;

    if (aDirDown) {
        if (rect->b > iViewRect.b) {
            iViewRect.t = rect->t;
            iViewRect.b = iViewRect.t + lcd.iHeight;
            if (iViewRect.b > h) {
                iViewRect.b = h;
                iViewRect.t = iViewRect.b - lcd.iHeight;
            }
        }
    }
    else {
        if (rect->t < iViewRect.t) {
            iViewRect.b = rect->b;
            iViewRect.t = iViewRect.b - lcd.iHeight;
            if (iViewRect.t < 0) {
                iViewRect.t = 0;
                iViewRect.b = lcd.iHeight;
            }
        }
    }
}

void CNBrKernel::PageUp()
{
    TSize lcd = iScreen->SizeInPixels();
    TInt h = page_height(GET_NBK_PAGE(iHistory));

    if (lcd.iHeight >= h)
        return;
    if (iViewRect.t == 0)
        return;

    int move = lcd.iHeight * MOVE_DISTANCE_BY_SCREEN;

    int t = iViewRect.t - move;
    if (t < 0)
        t = 0;

    iViewRect.t = t;
    iViewRect.b = iViewRect.t + lcd.iHeight;

    RefreshScreen();
}

void CNBrKernel::PageDown()
{
    TSize lcd = iScreen->SizeInPixels();
    TInt h = page_height(GET_NBK_PAGE(iHistory));

    if (lcd.iHeight >= h)
        return;
    if (iViewRect.b == h)
        return;

    int move = lcd.iHeight * MOVE_DISTANCE_BY_SCREEN;

    int t = iViewRect.b + move;
    if (t > h)
        t = h;
    iViewRect.b = t;
    iViewRect.t = iViewRect.b - lcd.iHeight;

    RefreshScreen();
}

void CNBrKernel::FocusNext()
{
    NBK_Page* page = GET_NBK_PAGE(iHistory);
    NNode* focus = (NNode*) view_getFocusNode(page->view);
    NRect r;

    if (focus == NULL) {
        focus = doc_getFocusNode(page->doc, (NRect*) &iViewRect);
    }
    else {
        r = view_getNodeRect(page->view, focus);
        if (rect_isIntersect((NRect*) &iViewRect, &r))
            focus = doc_getNextFocusNode(page->doc, focus);
        else
            focus = doc_getFocusNode(page->doc, (NRect*) &iViewRect);
    }

    if (focus) {
        r = view_getNodeRect(page->view, focus);
        AdjustViewRect((TNbkRect*) &r);
        view_setFocusNode(page->view, focus);
        RefreshScreen();
    }
    else
        PageDown();
}

void CNBrKernel::FocusPrev()
{
    NBK_Page* page = GET_NBK_PAGE(iHistory);
    NNode* focus = (NNode*) view_getFocusNode(page->view);
    NRect r;

    if (focus == NULL) {
        focus = doc_getFocusNode(page->doc, (NRect*) &iViewRect);
    }
    else {
        r = view_getNodeRect(page->view, focus);
        if (rect_isIntersect((NRect*) &iViewRect, &r))
            focus = doc_getPrevFocusNode(page->doc, focus);
        else
            focus = doc_getFocusNode(page->doc, (NRect*) &iViewRect);
    }

    if (focus) {
        r = view_getNodeRect(page->view, focus);
        AdjustViewRect((TNbkRect*) &r, EFalse);
        view_setFocusNode(page->view, focus);
        RefreshScreen();
    }
    else
        PageUp();
}

void CNBrKernel::FocusClick()
{
    NBK_Page* page = GET_NBK_PAGE(iHistory);

    if (ifCurZoomFactor >= KNbkDefaultZoomLevel) {
        void* focus = view_getFocusNode(page->view);
        doc_clickFocusNode(page->doc, (NNode*) focus);
    }
}

void CNBrKernel::ChangeZoomLevel(float aFactor)
{
    NBK_Page* page = GET_NBK_PAGE(iHistory);

    ifCurZoomFactor = aFactor;
    history_setZoom((NHistory*) iHistory, aFactor);

    TSize lcd = iScreen->SizeInPixels();
    NRect r;

    r.l = iArrowPos.iX;
    r.t = iArrowPos.iY;
    r.r = r.l + lcd.iWidth;
    r.b = r.t + lcd.iHeight;

    coord w = page_width(page);
    coord h = page_height(page);

    if (r.r > w) {
        r.r = w;
        r.l = r.r - lcd.iWidth;
        if (r.l < 0)
            r.l = 0;
    }
    if (r.b > h) {
        r.b = h;
        r.t = r.b - lcd.iHeight;
        if (r.t < 0)
            r.t = 0;
    }

    NBK_memcpy(&iViewRect, &r, sizeof(NRect));
    
    nid docType = doc_getType(page->doc);
    iViewXOffset = 0;

    if (docType == NEDOC_FULL) {
    }
    else if (docType == NEDOC_NARROW) {
        if (w < lcd.iWidth)
            iViewXOffset = (lcd.iWidth - w) >> 1;
    }
    else if (docType != NEDOC_MAINBODY) {
        if (ifCurZoomFactor < ifMinZoomfactor || fabs(ifCurZoomFactor - ifMinZoomfactor) < NFLOAT_PRECISION) {
            if (w < lcd.iWidth)
                iViewXOffset = (lcd.iWidth - w) >> 1;
        }
    }

    TPoint pt = iDrawOffset;
    pt.iX += iViewXOffset;
    iGdi->SetDrawOffset(pt);
    
    if (iAuto)
        RefreshScreen();
}

EXPORT_C TKeyResponse CNBrKernel::HandleKeyByControlL(const TKeyEvent &aKeyEvent, TEventCode aType)
{  
    TBool process;
    NEvent event;
    
    if (iDenyEvent)
        return EKeyWasNotConsumed;

    if (aType != EEventKey)
        return EKeyWasNotConsumed;
    
#if ENABLE_FEP
    if (iFepHandler && iFepHandler->EditMode())
#else
    CFEPControl* fepControl = CResourceManager::GetPtr()->GetFEPControl();
    if (fepControl && fepControl->IsEdit())
#endif
    {
        process = ETrue;
        NBK_memset(&event, 0, sizeof(NEvent));
        event.type = NEEVENT_KEY;
        event.page = GET_NBK_PAGE(iHistory);

        if (EKeyBackspace == aKeyEvent.iCode) {
            event.d.keyEvent.key = NEKEY_BACKSPACE;
        }
        else if (EKeyLeftArrow == aKeyEvent.iCode) {
            event.d.keyEvent.key = NEKEY_LEFT;
        }
        else if (EKeyRightArrow == aKeyEvent.iCode) {
            event.d.keyEvent.key = NEKEY_RIGHT;
        }
        else if (EKeyUpArrow == aKeyEvent.iCode) {
            event.d.keyEvent.key = NEKEY_UP;
        }
        else if (EKeyDownArrow == aKeyEvent.iCode) {
            event.d.keyEvent.key = NEKEY_DOWN;
        }
        else if (EKeyEnter == aKeyEvent.iCode || EKeyDevice3 == aKeyEvent.iCode) {
            event.d.keyEvent.key = NEKEY_ENTER;
        }
        else if (aKeyEvent.iCode >= 0x20 && aKeyEvent.iCode < ENonCharacterKeyBase) {
            event.type = NEEVENT_KEY;
            event.page = GET_NBK_PAGE(iHistory);
            event.d.keyEvent.key = NEKEY_CHAR;
            event.d.keyEvent.chr = aKeyEvent.iCode;
        }
        else {
            process = EFalse;
        }

        if (process) {

            if (doc_processKey(&event)) {
#if ENABLE_FEP
                iFepHandler->HandleUpdateCursor();
                iFepHandler->UpdateEditingMode();
#endif
            }
            else if (aKeyEvent.iScanCode == EStdKeyDevice3) {
                FocusClick();
            }
            else if (   aKeyEvent.iScanCode == EStdKeyUpArrow
                     || aKeyEvent.iScanCode == EStdKeyDownArrow) {
                FocusClick();
                return EKeyWasNotConsumed;
            }
        }
        return EKeyWasConsumed;
    }
    else {
        process = ETrue;
        NBK_memset(&event, 0, sizeof(NEvent));
        event.type = NEEVENT_KEY;
        event.page = GET_NBK_PAGE(iHistory);

        switch (aKeyEvent.iCode/*iScanCode*/) {
        case EKeyBackspace://EStdKeyBackspace:
            event.d.keyEvent.key = NEKEY_BACKSPACE;
            break;
        case EKeyLeftArrow://EStdKeyLeftArrow:
            event.d.keyEvent.key = NEKEY_LEFT;
            break;
        case EKeyRightArrow://EStdKeyRightArrow:
            event.d.keyEvent.key = NEKEY_RIGHT;
            break;
        case EKeyUpArrow://EStdKeyUpArrow:
            event.d.keyEvent.key = NEKEY_UP;
            break;
        case EKeyDownArrow://EStdKeyDownArrow:
            event.d.keyEvent.key = NEKEY_DOWN;
            break;
        case EKeyDevice3://EStdKeyDevice3:
            event.d.keyEvent.key = NEKEY_ENTER;
            break;
        default:
            process = EFalse;
        }

        if (process) {
            if (iTextSel && textSel_processEvent((NTextSel*)iTextSel, &event))
                return EKeyWasConsumed;
            if (doc_processKey(&event))
                return EKeyWasConsumed;
        }
        
        return EKeyWasNotConsumed;
    }
}

EXPORT_C TKeyResponse CNBrKernel::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
    if (iDenyEvent)
        return EKeyWasNotConsumed;

    if (HandleKeyByControlL(aKeyEvent, aType) == EKeyWasConsumed)
        return EKeyWasConsumed;

    i4Ways = (TBool)page_4ways(GET_NBK_PAGE(iHistory));

    if (i4Ways)
        return HandleKey4Ways(aKeyEvent, aType);
    else
        return HandleKey(aKeyEvent, aType);
}

TKeyResponse CNBrKernel::HandleKey(const TKeyEvent &aKeyEvent, TEventCode aType)
{
    TKeyResponse resp = EKeyWasConsumed;

    if (aType == EEventKey) {
        switch (aKeyEvent.iScanCode) {

        case EStdKeyLeftArrow:
            PageUp();
            break;

        case EStdKeyRightArrow:
            PageDown();
            break;

        case EStdKeyUpArrow:
            FocusPrev();
            break;

        case EStdKeyDownArrow:
            FocusNext();
            break;

        case EStdKeyDevice3:
            FocusClick();
            break;

        default:
            resp = EKeyWasNotConsumed;
        }
    }
    return resp;
}

TKeyResponse CNBrKernel::HandleKey4Ways(const TKeyEvent &aKeyEvent, TEventCode aType)
{
    TKeyResponse resp = EKeyWasConsumed;

    if (aType == EEventKey) {
        switch (aKeyEvent.iScanCode) {

        case EStdKeyLeftArrow:
            MoveLeft();
            break;

        case EStdKeyRightArrow:
            MoveRight();
            break;

        case EStdKeyUpArrow:
            MoveUp();
            break;

        case EStdKeyDownArrow:
            MoveDown();
            break;

        case EStdKeyDevice3:
            FocusClick();
            break;

        default:
            switch (aKeyEvent.iCode) {
            case '2':
            case 'w':
            case 'W':
            case 't':
            case 'T':
                MoveScreenUp(MOVE_DISTANCE_BY_SCREEN);
                break;
            case '4':
            case 'a':
            case 'A':
            case 'f':
            case 'F':
                MoveScreenLeft(MOVE_DISTANCE_BY_SCREEN);
                break;
            case '6':
            case 'd':
            case 'D':
            case 'h':
            case 'H':
                MoveScreenRight(MOVE_DISTANCE_BY_SCREEN);
                break;
            case '8':
            case 's':
            case 'S':
            case 'b':
            case 'B':
                MoveScreenDown(MOVE_DISTANCE_BY_SCREEN);
                break;
            default:
                resp = EKeyWasNotConsumed;
                break;
            }
            break;
        }
    }
    return resp;
}

EXPORT_C TBool CNBrKernel::HandlePointerByControlL(const TPointerEvent& aPointerEvent)
{
    int x = aPointerEvent.iPosition.iX;
    int y = aPointerEvent.iPosition.iY;
    x += iViewRect.l;
    y += iViewRect.t;

    NEvent event;
    NBK_memset(&event, 0, sizeof(NEvent));
    event.type = NEEVENT_PEN;
    event.page = GET_NBK_PAGE(iHistory);
    event.d.penEvent.pos.x = x;
    event.d.penEvent.pos.y = y;

    if (aPointerEvent.iType == TPointerEvent::EButton1Down)
        event.d.penEvent.type = NEPEN_DOWN;
    else if (aPointerEvent.iType == TPointerEvent::EButton1Up)
        event.d.penEvent.type = NEPEN_UP;
    else if (aPointerEvent.iType == TPointerEvent::EDrag)
        event.d.penEvent.type = NEPEN_MOVE;
    else
        return EFalse;

    if (iTextSel)
        return (TBool)textSel_processEvent((NTextSel*)iTextSel, &event);
    else
        return (TBool)doc_processPen(&event);
}

EXPORT_C void CNBrKernel::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
    if (iDenyEvent)
        return;

    iTouchScreen = ETrue;

    if (ifCurZoomFactor >= KNbkDefaultZoomLevel) {
        if (HandlePointerByControlL(aPointerEvent))
            return;
    }
    
    if (!iAuto)
        return;

    switch (aPointerEvent.iType) {
    case TPointerEvent::EButton1Down:
    {
        iLastPenPos = aPointerEvent.iPosition;
        TPoint pt(aPointerEvent.iPosition);
        pt.iX += iViewRect.l;
        pt.iY += iViewRect.t;
        iPenFocus = GetFocusByPos(pt);
        SetFocus(iPenFocus);
        RefreshScreen();
        break;
    }
    case TPointerEvent::EButton1Up:
    {
        if (   ifCurZoomFactor == ifMinZoomfactor
            && ifMinZoomfactor < KNbkDefaultZoomLevel) {
            // 检测双击操作
            int val = iDbcInterval;
            iDbcInterval = User::NTickCount();
            if (iDbcInterval - val < 200) {
                float zoom = history_getZoom((NHistory*)iHistory);
                iArrowPos.iX = float_idiv(iViewRect.l, zoom) + float_idiv(aPointerEvent.iPosition.iX, zoom);
                iArrowPos.iY = float_idiv(iViewRect.t, zoom) + float_idiv(aPointerEvent.iPosition.iY, zoom);
                ChangeZoomLevel(KNbkDefaultZoomLevel);
            }
            return;
        }

        TPoint pt(aPointerEvent.iPosition);
        pt.iX += iViewRect.l;
        pt.iY += iViewRect.t;

        if (iPenFocus) {
            if (iPenFocus == GetFocusByPos(pt))
                ClickFocus(iPenFocus);
        }

        RefreshScreen();
        break;
    }
    case TPointerEvent::EDrag:
    {
        int dx, dy;
        dx = aPointerEvent.iPosition.iX - iLastPenPos.iX;
        dy = aPointerEvent.iPosition.iY - iLastPenPos.iY;
        if (Abs(dx) > PEN_MOVE_MIN || Abs(dy) > PEN_MOVE_MIN) {
            iLastPenPos = aPointerEvent.iPosition;
            if (iPenFocus) {
                iPenFocus = NULL;
            }
            DragByPen(TPoint(-dx, -dy));
        }
        break;
    }
    default:
        break;
    }
}

void CNBrKernel::DragByPen(TPoint aMove)
{
    NPage* page = GET_NBK_PAGE(iHistory);
    TSize lcd = iScreen->SizeInPixels();
    coord w = page_width(page);
    coord h = page_height(page);
    coord dx = aMove.iX;
    coord dy = aMove.iY;
    bool update = false;
    TRect vr;
    NRect pr;

#if 1 // 局部刷新
    if (w > lcd.iWidth) {
        if (dx < 0)
            dx = (iViewRect.l + dx < 0) ? dx - (iViewRect.l + dx) : dx;
        else
            dx = (iViewRect.r + dx > w) ? dx - (iViewRect.r + dx - w) : dx;
        update = true;
    }
    else
        dx = 0;

    if (h > lcd.iHeight) {
        if (dy < 0)
            dy = (iViewRect.t + dy < 0) ? dy - (iViewRect.t + dy) : dy;
        else
            dy = (iViewRect.b + dy > h) ? dy - (iViewRect.b + dy - h) : dy;
        update = true;
    }
    else
        dy = 0;

    if (!update)
        return;

    // 水平移动
    if (dx) {
        NBK_memcpy(&pr, &iViewRect, sizeof(NRect));
        iViewRect.l += dx;
        iViewRect.r += dx;

        if (dx > 0) {
            // 向左移动
            iScreenGc->CopyRect(TPoint(-dx, 0), TRect(TPoint(dx, 0), lcd));
            pr.l += lcd.iWidth;
            pr.r = pr.l + dx;
            vr.SetRect(pr.l, pr.t, pr.r, pr.b);
            DrawPage(vr, TPoint(lcd.iWidth - dx, 0));
        }
        else if (dx < 0) {
            // 向右移动
            iScreenGc->CopyRect(TPoint(-dx, 0), TRect(TPoint(0, 0), lcd));
            pr.l += dx;
            pr.r = pr.l + Abs(dx);
            vr.SetRect(pr.l, pr.t, pr.r, pr.b);
            DrawPage(vr, TPoint(0, 0));
        }
    }

    // 垂直移动
    if (dy) {
        NBK_memcpy(&pr, &iViewRect, sizeof(NRect));
        iViewRect.t += dy;
        iViewRect.b += dy;

        if (dy > 0) {
            // 向上移动
            iScreenGc->CopyRect(TPoint(0, -dy), TRect(TPoint(0, dy), lcd));
            pr.t += lcd.iHeight;
            pr.b = pr.t + dy;
            vr.SetRect(pr.l, pr.t, pr.r, pr.b);
            DrawPage(vr, TPoint(0, lcd.iHeight - dy));
        }
        else if (dy < 0) {
            // 向下移动
            iScreenGc->CopyRect(TPoint(0, -dy), TRect(TPoint(0, 0), lcd));
            pr.t += dy;
            pr.b = pr.t + Abs(dy);
            vr.SetRect(pr.l, pr.t, pr.r, pr.b);
            DrawPage(vr, TPoint(0, 0));
        }
    }

    iShellInterface->NbkRedraw();

#else

    if (w > lcd.iWidth) {
        if (dx < 0)
        dx = (iViewRect.l + dx < 0) ? dx - (iViewRect.l + dx) : dx;
        else
        dx = (iViewRect.r + dx > w) ? dx - (iViewRect.r + dx - w) : dx;
        iViewRect.l += dx;
        iViewRect.r += dx;
        update = true;
    }

    if (h > lcd.iHeight) {
        if (dy < 0)
        dy = (iViewRect.t + dy < 0) ? dy - (iViewRect.t + dy) : dy;
        else
        dy = (iViewRect.b + dy > h) ? dy - (iViewRect.b + dy - h) : dy;
        iViewRect.t += dy;
        iViewRect.b += dy;
        update = true;
    }

    if (update)
    RefreshScreen();

#endif
}

void CNBrKernel::Clear()
{
    NBK_Page* page = GET_NBK_PAGE(iHistory);

    i4Ways = (TBool) page_4ways(page);
    iArrowPos.SetXY(ARROW_STOP, ARROW_STOP);
    iLastPos.SetXY(0, 0);
    ifCurZoomFactor = KNbkDefaultZoomLevel;
    ifMinZoomfactor = KNbkDefaultZoomLevel;
}

void CNBrKernel::SetPageWidth()
{
    TSize lcd = iScreen->SizeInPixels();
    NPage* page = GET_NBK_PAGE(iHistory);

#if FIXED_SCREEN_WIDTH
    page_setScreenWidth(page, SCREEN_WIDTH);
#else
    page_setScreenWidth(page, lcd.iWidth);
#endif
    iUseMinZoom = ETrue;
    CalcMinZoomFactor();

    if (iAuto) {
        page_layout(page, N_TRUE);
        CalcMinZoomFactor();
        UpdateViewRect();
        UpdateScreen();
    }
}

void CNBrKernel::RequestAP()
{
    iShellInterface->NbkRequestAP();
}

void CNBrKernel::Download(const TDesC8& aUrl, const TDesC16& aFileName, const TDesC8& aCookie, TInt aSize)
{
    iShellInterface->OnDownload(aUrl, aFileName, aCookie, aSize);
}

// -----------------------------------------------------------------------------
// 缩放功能
//
// 当最小缩放率大于1.0时，缩放操作以最小缩放率为基准进行
// -----------------------------------------------------------------------------

EXPORT_C void CNBrKernel::ZoomPage(float aZoom)
{
    if (ifMinZoomfactor > 1.0)
        aZoom *= ifMinZoomfactor;
    
    if (aZoom == ifCurZoomFactor)
        return;

    float fa = aZoom;
    if (fa < ifMinZoomfactor)
        fa = ifMinZoomfactor;
    if (fa > KNbkMaxZoomLevel)
        fa = KNbkMaxZoomLevel;

    // 以上次绘图左上角为缩放起点
    iArrowPos.iX = iViewRect.l;
    iArrowPos.iY = iViewRect.t;

    ChangeZoomLevel(fa);
}

EXPORT_C float CNBrKernel::GetCurrentZoomFactor(void)
{
    if (ifMinZoomfactor > 1.0)
        return ifCurZoomFactor / ifMinZoomfactor;
    else
        return ifCurZoomFactor;
}

EXPORT_C void CNBrKernel::SetCurrentZoomFactor(float aZoom)
{
    if (ifMinZoomfactor > 1.0)
        aZoom *= ifMinZoomfactor;
    
    if (aZoom >= ifMinZoomfactor && aZoom < KNbkMaxZoomLevel) {
        history_setZoom((NHistory*) iHistory, aZoom);
        ifCurZoomFactor = aZoom;
    }
}

float CNBrKernel::GetCurZoom()
{
    return ifCurZoomFactor;
}

void CNBrKernel::SetCurZoom(float aZoom)
{
    if (aZoom >= ifMinZoomfactor && aZoom < KNbkMaxZoomLevel) {
        history_setZoom((NHistory*) iHistory, aZoom);
        ifCurZoomFactor = aZoom;
    }
}

EXPORT_C float CNBrKernel::GetMinZoomFactor(void)
{
    if (ifMinZoomfactor > 1.0)
        return 1.0;
    else
        return ifMinZoomfactor;
}

EXPORT_C TInt CNBrKernel::SetIap(TUint32 aIapId, TBool aIsWap)
{
    return CResourceManager::GetPtr()->SetIap(aIapId, aIsWap);
}

EXPORT_C void CNBrKernel::SetAccessPoint(RConnection& aConnect, RSocketServ& aSocket,
    TUint32 aAccessPoint, const TDesC& aAcessName, const TDesC& aApnName, TBool aReady)
{
    CResourceManager::GetPtr()->SetAccessPoint(aConnect, aSocket, aAccessPoint, aAcessName, aApnName, aReady);
}

EXPORT_C TInt CNBrKernel::GetWidth() const
{
    return page_width(GET_NBK_PAGE(iHistory));
}

EXPORT_C TInt CNBrKernel::GetHeight() const
{
    return page_height(GET_NBK_PAGE(iHistory));
}

EXPORT_C void CNBrKernel::SetDrawParams(int aMaxTime, TBool aDrawPic, TBool aDrawBgimg)
{
    history_setPaintParams((NHistory*)iHistory, aMaxTime, aDrawPic, aDrawBgimg);
}

EXPORT_C void CNBrKernel::DrawPage(const TRect& aRect)
{
    DrawPage(aRect, TPoint(0, 0));
}

EXPORT_C void CNBrKernel::DrawPage(const TRect& aRect, const TPoint& aOffset)
{
#if DEBUG_SCROLL_VIEW
    if (iProbe&&aRect.iTl.iY == 0) {
        double t_time = NBK_currentMilliSeconds();
    }
#endif

    NRect vr;
    vr.l = aRect.iTl.iX;
    vr.t = aRect.iTl.iY;
    vr.r = aRect.iBr.iX;
    vr.b = aRect.iBr.iY;

    NPage* page = GET_NBK_PAGE(iHistory);
    TSize lcd = iScreen->SizeInPixels();
    int w = page_width(page);
    int h = page_height(page);

    // 局部绘制暂停动画
    if (aOffset == TPoint(0, 0))
        view_resume(page->view);
    else
        view_pause(page->view);

    // 清除未覆盖区域
    TRgb bgcolor(KRgbWhite);
    NSettings* settings = (NSettings*)iPageSettings;
    if (settings->night) {
        NNightTheme* night = (NNightTheme*)iNightTheme;
        bgcolor = TRgb((int)night->background.r, (int)night->background.g, (int)night->background.b);
    }
    if (w < lcd.iWidth) {
        if (iViewXOffset == 0) {
            // 清除右侧区域
            TRect r(w, 0, lcd.iWidth, lcd.iHeight);
            iScreenGc->SetBrushColor(bgcolor);
            iScreenGc->Clear(r);
        }
        else {
            // 清除左侧、右侧区域
            TRect r;
            iScreenGc->SetBrushColor(bgcolor);
            r.SetRect(0, 0, iViewXOffset, lcd.iHeight);
            iScreenGc->Clear(r);
            r.SetRect(iViewXOffset + w, 0, lcd.iWidth, lcd.iHeight);
            iScreenGc->Clear(r);
        }
    }
    if (h < lcd.iHeight) {
        TRect r(0, h, lcd.iWidth, lcd.iHeight);
        iScreenGc->SetBrushColor(bgcolor);
        iScreenGc->Clear(r);
    }

    iDrawOffset = aOffset;
    TPoint pt = aOffset;
    pt.iX += iViewXOffset;
    iGdi->SetDrawOffset(pt);
    page_paint(page, &vr);

    // 当存在主体框，且主体框宽度在可见区域内时，显示主体框
    NRect va;
    GetViewPort(&va);
    
    if (iTextSel)
        textSel_paint((NTextSel*)iTextSel, &vr);

#if DEBUG_SCROLL_VIEW
    TInt height = page_height(page);
    if (iProbe&&aRect.iBr.iY == height) {
        double t_time = NBK_currentMilliSeconds();
        iProbe->OutputChar("Scroll page consume time = ", -1);
        iProbe->OutputInt((int) (NBK_currentMilliSeconds() - t_time));
        iProbe->OutputReturn();
    }
#endif
}

void CNBrKernel::MoveLeft()
{
    int step = ARROW_MOVE_STEP, stop = ARROW_STOP;

    if (iArrowPos.iX > stop) {
        iArrowPos.iX -= step;
        if (iArrowPos.iX < 0)
            iArrowPos.iX = 0;

        CheckFocus();
        RefreshScreen();
    }
    else {
        MoveScreenLeft(MOVE_DISTANCE_BY_ARROW);
    }
}

void CNBrKernel::MoveScreenLeft(float ratio)
{
    TSize lcd = iScreen->SizeInPixels();
    TInt w = page_width(GET_NBK_PAGE(iHistory));

    if (lcd.iWidth >= w)
        return;
    if (iViewRect.l == 0)
        return;

    int move = lcd.iWidth * ratio;
    int l = iViewRect.l;
    int d = iViewRect.l - move;
    if (d < 0)
        d = 0;
    iViewRect.l = d;
    iViewRect.r = iViewRect.l + lcd.iWidth;
    iArrowPos.iX = l - d;

    CheckFocus();
    RefreshScreen();
}

void CNBrKernel::MoveRight()
{
    TSize lcd = iScreen->SizeInPixels();
    int step = ARROW_MOVE_STEP, stop = ARROW_STOP;

    if (iArrowPos.iX + step < lcd.iWidth - stop) {
        iArrowPos.iX += step;
        if (iArrowPos.iX >= lcd.iWidth)
            iArrowPos.iX = lcd.iWidth - 1;

        CheckFocus();
        RefreshScreen();
    }
    else {
        MoveScreenRight(MOVE_DISTANCE_BY_ARROW);
    }
}

void CNBrKernel::MoveScreenRight(float ratio)
{
    TSize lcd = iScreen->SizeInPixels();
    TInt w = page_width(GET_NBK_PAGE(iHistory));

    if (lcd.iWidth >= w)
        return;
    if (iViewRect.r == w)
        return;

    int move = lcd.iWidth * ratio;
    int r = iViewRect.r;
    int d = iViewRect.r + move;
    if (d > w)
        d = w;
    iViewRect.r = d;
    iViewRect.l = iViewRect.r - lcd.iWidth;
    iArrowPos.iX = lcd.iWidth - (d - r);

    CheckFocus();
    RefreshScreen();
}

void CNBrKernel::MoveUp()
{
    int step = ARROW_MOVE_STEP, stop = ARROW_STOP;

    if (iArrowPos.iY > stop) {
        iArrowPos.iY -= step;
        if (iArrowPos.iY < 0)
            iArrowPos.iY = 0;

        CheckFocus();
        RefreshScreen();
    }
    else {
        MoveScreenUp(MOVE_DISTANCE_BY_ARROW);
    }
}

void CNBrKernel::MoveScreenUp(float ratio)
{
    TSize lcd = iScreen->SizeInPixels();
    TInt h = page_height(GET_NBK_PAGE(iHistory));

    if (lcd.iHeight >= h)
        return;
    if (iViewRect.t == 0)
        return;

    int move = lcd.iHeight * ratio;
    int t = iViewRect.t;
    int d = iViewRect.t - move;
    if (d < 0)
        d = 0;
    iViewRect.t = d;
    iViewRect.b = iViewRect.t + lcd.iHeight;
    iArrowPos.iY = t - d;

    CheckFocus();
    RefreshScreen();
}

void CNBrKernel::MoveDown()
{
    TSize lcd = iScreen->SizeInPixels();
    int step = ARROW_MOVE_STEP, stop = ARROW_STOP;

    if (iArrowPos.iY + step < lcd.iHeight - stop) {
        iArrowPos.iY += step;
        if (iArrowPos.iY >= lcd.iHeight)
            iArrowPos.iY = lcd.iHeight - 1;

        CheckFocus();
        RefreshScreen();
    }
    else {
        MoveScreenDown(MOVE_DISTANCE_BY_ARROW);
    }
}

void CNBrKernel::MoveScreenDown(float ratio)
{
    TSize lcd = iScreen->SizeInPixels();
    TInt h = page_height(GET_NBK_PAGE(iHistory));

    if (lcd.iHeight >= h)
        return;
    if (iViewRect.b == h)
        return;

    int move = lcd.iHeight * ratio;
    int b = iViewRect.b;
    int d = iViewRect.b + move;
    if (d > h)
        d = h;
    iViewRect.b = d;
    iViewRect.t = iViewRect.b - lcd.iHeight;
    iArrowPos.iY = lcd.iHeight - (d - b);

    CheckFocus();
    RefreshScreen();
}

void CNBrKernel::CheckFocus()
{
    // 处于适屏模式时，不检测焦点
    if (ifCurZoomFactor == ifMinZoomfactor && ifMinZoomfactor < KNbkDefaultZoomLevel)
        return;

    NBK_Page* page = GET_NBK_PAGE(iHistory);
    TInt x, y;

    x = iViewRect.l + iArrowPos.iX;
    y = iViewRect.t + iArrowPos.iY;

    NNode* focus = doc_getFocusNodeByPos(page->doc, x, y);
    view_setFocusNode(page->view, focus);
}

EXPORT_C void* CNBrKernel::GetFocus(const TRect& aRect)
{
    NRect r;
    r.l = aRect.iTl.iX;
    r.t = aRect.iTl.iY;
    r.r = aRect.iBr.iX;
    r.b = aRect.iBr.iY;

    return doc_getFocusNode(GET_NBK_PAGE(iHistory)->doc, &r);
}

EXPORT_C void* CNBrKernel::GetFocus()
{
    return view_getFocusNode(GET_NBK_PAGE(iHistory)->view);
}

EXPORT_C void* CNBrKernel::GetFocusByPos(const TPoint& aPos)
{
    return doc_getFocusNodeByPos(GET_NBK_PAGE(iHistory)->doc, aPos.iX - iViewXOffset, aPos.iY);
}

EXPORT_C void* CNBrKernel::GetNextFocus(void* focus)
{
    return doc_getNextFocusNode(GET_NBK_PAGE(iHistory)->doc, (NNode*) focus);
}

EXPORT_C void* CNBrKernel::GetPrevFocus(void* focus)
{
    return doc_getPrevFocusNode(GET_NBK_PAGE(iHistory)->doc, (NNode*) focus);
}

EXPORT_C void CNBrKernel::GetFocusRect(void* focus, TRect& aRect)
{
    NRect r;
    doc_getFocusNodeRect(GET_NBK_PAGE(iHistory)->doc, (NNode*) focus, &r);

    aRect.iTl.iX = r.l;
    aRect.iTl.iY = r.t;
    aRect.iBr.iX = r.r;
    aRect.iBr.iY = r.b;
}

EXPORT_C void CNBrKernel::SetFocus(void* focus)
{
    page_setFocusedNode(GET_NBK_PAGE(iHistory), (NNode*) focus);
}

EXPORT_C void CNBrKernel::SetCtrlNodeState(
    void* aFocusNode, MNbkCtrlPaintInterface::TCtrlState aState)
{
    if (page_isCtrlNode(GET_NBK_PAGE(iHistory), (NNode*) aFocusNode)) {
        iNodeState = aState;
    }
}

EXPORT_C void CNBrKernel::ClickFocus(void* focus)
{
    doc_clickFocusNode(GET_NBK_PAGE(iHistory)->doc, (NNode*) focus);
}

EXPORT_C const TPtrC8 CNBrKernel::GetUrl() const
{
    TPtrC8 url(NULL, 0);
    NPage* page = GET_NBK_PAGE(iHistory);
    char* url_c = history_getUrl((NHistory*)iHistory, page->id);
    if (url_c)
        url.Set((TUint8*) url_c, nbk_strlen(url_c));
    return url;
}

EXPORT_C const TPtrC CNBrKernel::GetTitle() const
{
    TPtrC title(NULL, 0);
    wchr* title_w = doc_getTitle(GET_NBK_PAGE(iHistory)->doc);
    if (title_w)
        title.Set((TUint16*) title_w, nbk_wcslen(title_w));
    return title;
}

EXPORT_C const CNBrKernel::EDocType CNBrKernel::GetType() const
{
    return (EDocType)doc_getType(GET_NBK_PAGE(iHistory)->doc);
}

EXPORT_C HBufC8* CNBrKernel::GetFocusUrl(void* focus)
{
    HBufC8* url = NULL;
    char* url_c = doc_getFocusUrl(GET_NBK_PAGE(iHistory)->doc, (NNode*) focus);
    if (url_c) {
        url = HBufC8::New(nbk_strlen(url_c) + 1);
        *url = (TUint8*) url_c;
        NBK_free(url_c);
    }
    return url;
}

// 设置内核工作模式
EXPORT_C void CNBrKernel::SetWorkMode(EWorkMode aMode)
{
    NSettings* settings = (NSettings*) iPageSettings;
    NPage* page = GET_NBK_PAGE(iHistory);

    switch (aMode) {
    case EDirect:
        settings->initMode = NEREV_STANDARD;
        i4Ways = EFalse;
        break;

    case EFffull:
        settings->initMode = NEREV_FF_FULL;
        i4Ways = ETrue;
        break;

    case EFfNarrow:
        settings->initMode = NEREV_FF_NARROW;
        i4Ways = ETrue;
        break;

    case EUck:
        settings->initMode = NEREV_UCK;
        i4Ways = EFalse;
        break;
    }
}

// 获取内核工作模式
EXPORT_C CNBrKernel::EWorkMode CNBrKernel::GetWorkMode() const
{
    int mode = ((NSettings*) iPageSettings)->initMode;

    switch (mode) {
    case NEREV_STANDARD:
        return EDirect;
    case NEREV_FF_FULL:
        return EFffull;
    case NEREV_FF_NARROW:
        return EFfNarrow;
    case NEREV_UCK:
        return EUck;
    default:
        return EUnknown;
    }
}

EXPORT_C void CNBrKernel::SetWorkModeForTC(EWorkMode aMode)
{
    NSettings* settings = (NSettings*) iPageSettings;

    if (aMode == EFffull)
        settings->mode = NEREV_FF_FULL;
    else if (aMode == EUck)
        settings->mode = NEREV_UCK;
    else if (aMode == EFfNarrow)
        settings->mode = NEREV_FF_NARROW;
}

EXPORT_C TBool CNBrKernel::Is4Ways() const
{
    return (TBool) page_4ways(GET_NBK_PAGE(iHistory));
}

EXPORT_C void CNBrKernel::FepInit(CCoeControl* aObjectProvider)
{
    iObjectProvider = aObjectProvider;
#if ENABLE_FEP
    //return;
    iFepHandler = CFEPHandler::NewL(iObjectProvider);
    iFepHandler->SetPage(GET_NBK_PAGE(iHistory));
    iFepHandler->iProbe = iProbe;

    iPlatform.fep = iFepHandler;
#endif
}

EXPORT_C TCoeInputCapabilities CNBrKernel::InputCapabilities() const
{
#if ENABLE_FEP
    if (iFepHandler)
        return iFepHandler->InputCapabilities();
#endif    
    return  TCoeInputCapabilities::ENone;
}

EXPORT_C void CNBrKernel::PasteText(const TDesC& aText)
{
#if ENABLE_FEP
#else
    CFEPControl* fepControl = CResourceManager::GetPtr()->GetFEPControl();
    if (fepControl) {
        if (fepControl->IsEdit())
        {
            fepControl->SetText(aText, ETrue);
            fepControl->DrawNow();
        }
        else
            fepControl->PasteTextL(this, aText);
    }
#endif
}

EXPORT_C void CNBrKernel::Dump(char* string, int length)
{
    if (iProbe) {
        iProbe->OutputChar(string, length);
        iProbe->OutputReturn();
    }
}

#ifdef __NBK_SELF_TESTING__

EXPORT_C void CNBrKernel::StartMonkey(const TDesC8& aUrl, EWorkMode aMode, eMonkeyType aType,
    TInt aEventInterval, TInt aEventCnt, TUid aUid)
{
    SetWorkMode(aMode);
    if (!iMonkeyTester)
    iMonkeyTester = new (ELeave) CMonkeyTester(this,aUid);

    iMonkeyTester->Start(aUrl, aType, aEventCnt);
}

EXPORT_C void CNBrKernel::StopMonkey()
{
    if (iMonkeyTester) {
        iMonkeyTester->Stop();
        ResetMonkey();
    }
}

void CNBrKernel::ResetMonkey(void)
{
    if (iMonkeyTester) {
        delete iMonkeyTester;
        iMonkeyTester = NULL;
    }
}

#endif

EXPORT_C void CNBrKernel::Stop()
{
    NBK_Page* page = GET_NBK_PAGE(iHistory);
    page_stop(page);
}

EXPORT_C void CNBrKernel::SetAllowImage(TBool aEnable)
{
    ((NSettings*)iPageSettings)->allowImage = (aEnable) ? 1 : 0;
    history_enablePic((NHistory*)iHistory, ((NSettings*)iPageSettings)->allowImage);
}

EXPORT_C void CNBrKernel::SetSelfAdaptionLayout(TBool aEnable)
{
    ((NSettings*)iPageSettings)->selfAdaptionLayout = (aEnable) ? 1 : 0;
}

EXPORT_C TBool CNBrKernel::AllowImage() const
{
    return (TBool) ((NSettings*)iPageSettings)->allowImage;
}

EXPORT_C void CNBrKernel::SetFontSize(TInt aPixel, TInt aLevel)
{
    ((NSettings*)iPageSettings)->mainFontSize = (int16)aPixel;
    
    int8 level;
    if (aLevel == EFontSmall)
        level = NEFNTLV_SMALL;
    else if (aLevel == EFontMiddle)
        level = NEFNTLV_MIDDLE;
    else
        level = NEFNTLV_BIG;
    ((NSettings*)iPageSettings)->fontLevel = level;
}

EXPORT_C TInt CNBrKernel::GetFontSize() const
{
    return (TInt) ((NSettings*)iPageSettings)->mainFontSize;
}

EXPORT_C void CNBrKernel::SetFontAntiAliasing(TBool aAnti)
{
    iGdi->SetAntiAliasing(aAnti);
}

EXPORT_C TBool CNBrKernel::IsFontAntiAliasing(void)
{
    return iGdi->IsAntiAliasing();
}

EXPORT_C TBool CNBrKernel::Back()
{
    TBool ret = EFalse;

    if (iAuto) {
        if (ifCurZoomFactor > ifMinZoomfactor) {
            float zoom = history_getZoom((NHistory*)iHistory);
            coord x = float_idiv(iViewRect.l, zoom);
            coord y = float_idiv(iViewRect.t, zoom);
            x = float_imul(x, ifMinZoomfactor);
            y = float_imul(y, ifMinZoomfactor);
            iArrowPos.iX = x;
            iArrowPos.iY = y;
            ChangeZoomLevel(ifMinZoomfactor);
        }
        else
            ret = (TBool)history_prev((NBK_History*)iHistory);
    }
    else
        ret = (TBool)history_prev((NBK_History*)iHistory);

    return ret;
}

EXPORT_C TBool CNBrKernel::Forward()
{
    TBool ret = (TBool) history_next((NBK_History*) iHistory);
    return ret;
}

EXPORT_C void CNBrKernel::GetHistoryRange(TInt& aCurrent, TInt& aAll)
{
    NBK_History* h = (NBK_History*) iHistory;
    int16 a, b;
    history_getRange(h, &a, &b);
    aCurrent = (TInt) a;
    aAll = (TInt) b;
}

EXPORT_C TBool CNBrKernel::GotoHistoryPage(TInt aWhere)
{
    int16 w = (TInt) aWhere;
    return (TBool) history_goto((NBK_History*) iHistory, w);
}

EXPORT_C void CNBrKernel::SetControlPainter(MNbkCtrlPaintInterface* aPainter)
{
    iPlatform.ctlPainter = aPainter;
}

EXPORT_C void CNBrKernel::SetDialogInterface(MNbkDialogInterface* aInterface)
{
    iPlatform.dlgImpl = aInterface;
}

EXPORT_C void CNBrKernel::ClearCookies()
{
    CResourceManager::GetPtr()->ClearCookies();
}

EXPORT_C void CNBrKernel::Pause()
{
    view_pause(GET_NBK_PAGE(iHistory)->view);

    CFEPControl* fepControl = CResourceManager::GetPtr()->GetFEPControl();
    if (fepControl && fepControl->IsEdit()) {
        ClickFocus(GetFocus());
        iEditPaused = ETrue;
    }
}

EXPORT_C void CNBrKernel::Resume()
{
    view_resume(GET_NBK_PAGE(iHistory)->view);
    NNode* focus = (NNode*) GetFocus();
    if (iEditPaused && focus && (TAGID_INPUT == focus->id || TAGID_TEXTAREA == focus->id)) {
        ClickFocus(focus);
        iEditPaused = EFalse;
    }
}

EXPORT_C void CNBrKernel::Refresh()
{
    page_refresh(GET_NBK_PAGE(iHistory));
}

void CNBrKernel::RePos(int aX, int aY)
{
    TSize lcd = iScreen->SizeInPixels();
    int w = page_width(GET_NBK_PAGE(iHistory));
    int h = page_height(GET_NBK_PAGE(iHistory));

    if (i4Ways) {
        iViewRect.l = aX;
        iViewRect.r = iViewRect.l + lcd.iWidth;
        if (iViewRect.r > w) {
            iViewRect.r = w;
            iViewRect.l = w - lcd.iWidth;
            if (iViewRect.l < 0)
                iViewRect.l = 0;
        }

        iViewRect.t = aY;
        iViewRect.b = iViewRect.t + lcd.iHeight;
        if (iViewRect.b > h) {
            iViewRect.b = h;
            iViewRect.t = h - lcd.iHeight;
            if (iViewRect.t < 0)
                iViewRect.t = 0;
        }
    }
    else {
        if (lcd.iHeight >= h)
            return;

        TInt y = (TInt) aY;
        if (y + lcd.iHeight > h)
            y = h - lcd.iHeight;
        iViewRect.t = y;
        iViewRect.b = y + lcd.iHeight;
    }
}

TBool CNBrKernel::SelectFile(TDesC& aOldFile, HBufC*& aSelectedFile)
{
    return iShellInterface->OnSelectFile(aOldFile, aSelectedFile);
}

void CNBrKernel::GetViewPort(void* aRect)
{
    if (iAuto) {
        NRect* r = (NRect*) aRect;
        TSize lcd = iScreen->SizeInPixels();

        r->l = iViewRect.l;
        r->t = iViewRect.t;
        r->r = r->l + lcd.iWidth;
        r->b = r->t + lcd.iHeight;
    }
    else {
        NRect* r = (NRect*) aRect;
        TRect viewport;

        iShellInterface->OnGetDocViewPort(viewport);

        r->l = viewport.iTl.iX;
        r->t = viewport.iTl.iY;
        r->r = viewport.iBr.iX;
        r->b = viewport.iBr.iY;
    }
}

void CNBrKernel::GetNBKOffset(void* aPoint)
{
    if (iAuto) {
        NPoint* p = (NPoint*) aPoint;
        TPoint offset(0,52);
        iShellInterface->OnGetNBKOffset(offset);
        p->x = offset.iX;
        p->y = offset.iY;
    }
    else {
        NPoint* p = (NPoint*) aPoint;
        TPoint offset(0,52);
        iShellInterface->OnGetNBKOffset(offset);
        p->x = offset.iX;
        p->y = offset.iY;
    }  
}

void* CNBrKernel::GetPage(void)
{
    return GET_NBK_PAGE(iHistory);
}

void CNBrKernel::OnEditStateChange(TBool aEdit)
{
    if (iAuto) {
    }
    else {
        iShellInterface->OnEditorStateChange(aEdit);
    }
}

TBool CNBrKernel::IsNightTheme(void)
{
    NSettings* settings = (NSettings*) iPageSettings;
    TBool res = settings->night ? ETrue : EFalse;
    return res;
}

// 调用页面排版
EXPORT_C void CNBrKernel::Layout(TBool bForce)
{
    page_layout(GET_NBK_PAGE(iHistory), (nbool) bForce);
    iUseMinZoom = ETrue;
    CalcMinZoomFactor();
}

// 设置夜间模式的颜色
EXPORT_C void CNBrKernel::SetNightTheme(TRgb aText, TRgb aLink, TRgb aBg)
{
    NNightTheme* theme = (NNightTheme*)iNightTheme;
    color_set(&theme->text, (uint8)aText.Red(), (uint8)aText.Green(), (uint8)aText.Blue(), 0xff);
    color_set(&theme->link, (uint8)aLink.Red(), (uint8)aLink.Green(), (uint8)aLink.Blue(), 0xff);
    color_set(&theme->background, (uint8)aBg.Red(), (uint8)aBg.Green(), (uint8)aBg.Blue(), 0xff);
}

// 使用夜间模式
EXPORT_C void CNBrKernel::UseNightTheme(TBool aUse)
{
    NSettings* settings = (NSettings*)iPageSettings;
    settings->night = (aUse) ? (NNightTheme*)iNightTheme : N_NULL;
    CResourceManager::GetPtr()->UseNightTheme(aUse);
}

EXPORT_C void CNBrKernel::CopyModeBegin()
{
    N_ASSERT(iTextSel == N_NULL);
    iTextSel = textSel_begin(GET_NBK_PAGE(iHistory)->view);
}

EXPORT_C void CNBrKernel::CopyModeEnd()
{
    N_ASSERT(iTextSel);
    NTextSel* sel = (NTextSel*)iTextSel;
    textSel_end(&sel);
    iTextSel = N_NULL;
}

EXPORT_C void CNBrKernel::SetCopyStartPoint(const TPoint& aPoint)
{
    N_ASSERT(iTextSel);
    NPoint pt = {aPoint.iX, aPoint.iY};
    NTextSel* sel = (NTextSel*)iTextSel;
    textSel_setStartPoint(sel, &pt);
}

EXPORT_C void CNBrKernel::SelectTextByKey(const TPoint& aPoint)
{
    N_ASSERT(iTextSel);
    NPoint pt = {aPoint.iX, aPoint.iY};
    NTextSel* sel = (NTextSel*)iTextSel;
    textSel_useKey(sel, &pt);
}

EXPORT_C HBufC* CNBrKernel::GetSelectedText()
{
    HBufC* buf = NULL;
    if (iTextSel) {
        int len;
        wchr* txt = textSel_getText((NTextSel*)iTextSel, &len);
        if (txt) {
            buf = HBufC::NewL(len);
            buf->Des().Copy(txt, len);
            NBK_free(txt);
        }
    }
    return buf;
}

EXPORT_C CNBrKernel::ERenderKind CNBrKernel::GetRenderKindByPos(const TPoint& aPos)
{
    NPoint pt = {aPos.iX, aPos.iY};
    NPage* page = GET_NBK_PAGE(iHistory);
    NRenderNode* r = view_findRenderByPos(page->view, &pt);
    return (ERenderKind)view_getRenderKind(r);
}

EXPORT_C TPtrC8 CNBrKernel::GetImageSrcByPos(const TPoint& aPos)
{
    NPoint pt = {aPos.iX, aPos.iY};
    NPage* page = GET_NBK_PAGE(iHistory);
    NRenderNode* r = view_findRenderByPos(page->view, &pt);
    const char* src = view_getRenderSrc(r);
    return TPtrC8((const TUint8*)src, nbk_strlen(src));
}

TDisplayMode CNBrKernel::GetDisplayMode()
{
    if (iScreen)
        return iScreen->DisplayMode();
    return EColor64K;
}
