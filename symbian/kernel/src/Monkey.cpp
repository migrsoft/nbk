#include <e32base.h>
#include <apgtask.h>
#include <e32math.h>
#include "../../../stdc/inc/nbk_cbDef.h"
#include "../../../stdc/dom/history.h"
#include "../../../stdc/dom/page.h"
#include "../../../stdc/dom/document.h"
#include "../../../stdc/tools/slist.h"
#include "Monkey.h"

RWsSession CMonkeyTester::wsSession;
TBool CMonkeyTester::iInited = EFalse;

TBool MonkeyMouseEvent::SimulateEvent(void)
{
    TRawEvent lEvent;
    lEvent.Set(iType, iPos.iX, iPos.iY);
    UserSvr::AddEvent(lEvent);
    return ETrue;
}

MonkeyMouseEvent::MonkeyMouseEvent(TRawEvent::TType aType, const TPoint& aPos) :
    MonkeyEvent(MonkeyEvent::EType_Mouse), iType(aType), iPos(aPos)
{
}

void CMonkeyTester::OnNbkEvent(CNBrKernel::TNbkEvent& aEvent)
{
    switch (aEvent.eventId) {

    case NBK_EVENT_DOWNLOAD_IMAGE:
    {
        if (aEvent.imgCur < aEvent.imgTotal) {
            if (!iWaitLoadFinish)
                RunMonkey();
        }
        else {
            if (iWaitLoadFinish)
                RunMonkey();
        }
        break;
    }
        // 接收到数据，开始建立文档
    case NBK_EVENT_NEW_DOC_BEGIN:
        // 收到服务器回应
    case NBK_EVENT_GOT_RESPONSE:
        // 接收到数据
    case NBK_EVENT_GOT_DATA:
        // 文档排版完成
    case NBK_EVENT_LAYOUT_FINISH:
    default:
        break;
    }
}

CMonkeyTester::CMonkeyTester(CNBrKernel* aNbrKernel, TUid aUid) :
    iNbrKernel(aNbrKernel), iUid(aUid), iMode(CNBrKernel::eHitPageLinks), iMonkeyRun(EFalse),
        iUrlLoadCnt(0), iMonkeyEventInterval(1000), iMonkeyEventCnt(10000), iWaitLoadFinish(ETrue),
        iFixedMonkeyUrlSeted(EFalse),iTimer(NULL)
{
}

CMonkeyTester::~CMonkeyTester(void)
{
    ResetUrl();
    if (CNBrKernel::eRandomEvent == iMode) {
        ResetRandom();
        iRandomEventQueue.ResetAndDestroy();
    }
}

TBool CMonkeyTester::Start(const TDesC8& aUrl, CNBrKernel::eMonkeyType aMode, TInt aEventCnt)
{
    iMode = aMode;
    iMonkeyEventCnt = aEventCnt;
    iUrlLoadCnt = 0;

    if (CNBrKernel::eHitPageLinks == iMode) {
        if(aUrl.Length())
            iFixedMonkeyUrl = aUrl.AllocL();
    }
    else if (CNBrKernel::eRandomEvent == iMode) {

        InitRandom();
        iRandomEventPercent[ETouch] = 50;
        iRandomEventPercent[EMove] = 100;
        iRandomEventPercent[EKeyNavigation] = 70;
        iRandomEventPercent[EKeyOk] = 95;
        iRandomEventPercent[EOtherKey] = 100;
    }
    if (iNbrKernel) {
        iMonkeyRun = ETrue;
        iNbrKernel->LoadUrl(aUrl);
    }
}

void CMonkeyTester::Stop(void)
{
    iMonkeyRun = EFalse;
    ResetUrl();

    if (CNBrKernel::eRandomEvent == iMode)
        ResetRandom();
}

void CMonkeyTester::SetOpt(eMonkeyOpt aType, TInt aValue)
{
    switch (aType) {
    case eWaitLoadFinish:
        iWaitLoadFinish = (TBool) aValue;
        break;
    case eMonkeyEventInterval:
        iMonkeyEventInterval = aValue;
        break;
    default:
        break;
    }
}

void CMonkeyTester::RunMonkey(void)
{
    if (!iMonkeyRun)
        return;

    if (CNBrKernel::eHitPageLinks == iMode) {
        DoHitPageLinks();
    }
    else if (CNBrKernel::eRandomEvent == iMode) {
        if (!iTimer)
            iTimer = CPeriodic::NewL(CActive::EPriorityStandard - 1);
        iNbrKernel->ZoomPage(0.8);
        DoRandomMonkey();
        iUrlLoadCnt++;
    }
}

void CMonkeyTester::ResetUrl(void)
{
    if (iFixedMonkeyUrl) {
        delete iFixedMonkeyUrl;
        iFixedMonkeyUrl = NULL;
    }
}

void CMonkeyTester::DoHitPageLinks()
{
    TPtrC8 pageUrl = iNbrKernel->GetUrl();
    if (!iFixedMonkeyUrlSeted) {
        iFixedMonkeyUrlSeted = ETrue;
        if(iFixedMonkeyUrl->Compare(pageUrl))
        {
            ResetUrl();
            iFixedMonkeyUrl = pageUrl.AllocL();
        }
    }

    if (iFixedMonkeyUrl && iFixedMonkeyUrl->Compare(pageUrl))
        iNbrKernel->Back();
    else {
        iUrlLoadCnt++;
        NPage* page = (history_curr((NHistory*) iNbrKernel->iHistory));
        if (page) {
            NDocument* doc = page->doc;
            nid tags[] = { TAGID_A, 0 };
            NSList* aNodeLst;
            aNodeLst = node_getListByTag(doc->root, tags, ((NPage*) doc->page)->view->path);
            if (aNodeLst) {
                NNode* n = (NNode*) sll_first(aNodeLst);
                for (TInt i = 1; n && i < iUrlLoadCnt; i++)
                    n = (NNode*) sll_next(aNodeLst);

                if (n)
                    iNbrKernel->ClickFocus(n);

                sll_delete(&aNodeLst);
            }
        }
    }
}

TInt CMonkeyTester::GetRangNum(TInt startNum, TInt endNum)
{
    TTime theTime(startNum);
    theTime.UniversalTime();
    TInt64 randSeed(theTime.Int64());
    TInt number(startNum + Math::Rand(randSeed) % (endNum - startNum));

    return number;
}

TInt CMonkeyTester::OnTimer(TAny* ptr)
{
    CMonkeyTester* self = (CMonkeyTester*)ptr;
    self->iTimer->Cancel();
    self->DoRandomMonkey();
    return 0;
}

void CMonkeyTester::InitRandom(void)
{
    if (!iInited) {
        wsSession.Connect();
        iInited = ETrue;
    }
}

void CMonkeyTester::UninitRandom(void)
{
    if (iInited) {
        wsSession.Close();
        iInited = EFalse;
    }
}

void CMonkeyTester::ResetRandom(void)
{
    if (iTimer) {
         iTimer->Cancel();
         delete iTimer;
         iTimer = NULL;
    }
    UninitRandom();
}

MonkeyEvent* CMonkeyTester::NextEvent()
{
    if (iRandomEventQueue.Count() == 0) {
        MakeRandomEvent();
    }
    MonkeyEvent* event = iRandomEventQueue[0];
    iRandomEventQueue.Remove(0);
    return event;
}

void CMonkeyTester::MakeRandomEvent()
{
    TInt num = GetRangNum(1,10000);
    TInt cls = num / 100;
    TInt lastKey = 0;

    TBool touchEvent = EFalse;
    TBool move = EFalse;
    if(cls <= iRandomEventPercent[ETouch])
        touchEvent = ETrue;
    else if (cls > iRandomEventPercent[ETouch] && cls <= iRandomEventPercent[EMove])
    { 
        touchEvent = ETrue;
        move = ETrue;
    }
    if (touchEvent) {
        MakeMouseEvent(move);
        return;
    }

    //    // The remaining event categories are injected as key events
    //    if (cls < iRandomEventPercent[EKeyNavigation]) {
    //        lastKey = MonkeyKeyEvent::NavKeyScanCode[rand() % MonkeyKeyEvent::NavKeyScanNum];
    //    }
    //    else if (cls < iRandomEventPercent[EKeyOk]) {
    //        lastKey = MonkeyKeyEvent::MajorNavKeyScanCode[rand() % MonkeyKeyEvent::MajorNavKeyScanNum];
    //    }
    //    else if (cls < iRandomEventPercent[EOtherKey]) {
    //        lastKey = MonkeyKeyEvent::KeySysScanCode[rand() % MonkeyKeyEvent::KeySysScanNum];
    //    }
    //    else {
    //        lastKey = MonkeyKeyEvent::KeyAlphaScanCode[rand() % MonkeyKeyEvent::KeyAlphaScanNum];
    //    }
    //    
    //     MonkeyKeyEvent* p = new MonkeyKeyEvent(0, lastKey);
    //     iRandomEventQueue.Append(p);
}

void CMonkeyTester::MakeMouseEvent(TBool aMove)
{
    CWsScreenDevice* screenDev = CCoeEnv::Static()->ScreenDevice();
    TSize size = screenDev->SizeInPixels();

    TPoint pos(0, 0);
    pos.iX = GetRangNum(1, size.iWidth);
    pos.iY = GetRangNum(1, size.iHeight * 0.9);

    MonkeyMouseEvent* event = new (ELeave) MonkeyMouseEvent(TRawEvent::EButton1Down, pos);

    iRandomEventQueue.Append(event);

    // sometimes we'll move during the touch
    if (aMove) {
        int count = 1;//GetRangNum(1, 2);
        for (int i = 0; i < count; i++) {
            // generate some slop in the up event
            pos.iX = ((TInt) pos.iX + GetRangNum(50, 150)) % size.iWidth;
            pos.iY = ((TInt) pos.iY + GetRangNum(80, 200)) % size.iHeight;

            event = new (ELeave) MonkeyMouseEvent(TRawEvent::EPointerMove, pos);
            iRandomEventQueue.Append(event);
        }
    }
    // TODO generate some slop in the up event
    event = new (ELeave) MonkeyMouseEvent(TRawEvent::EButton1Up, pos);
    iRandomEventQueue.Append(event);
}

TUint CMonkeyTester::DoRandomMonkey()
{
    int i = 0;
    int lastKey = 0;

    TBool systemCrashed = false;

    while (iMonkeyRun && i < iMonkeyEventCnt) {
        MonkeyEvent* ev = NextEvent();
        if (ev) {
            TBool res = ev->SimulateEvent();
            FindAndShow();        
            if (!res) {
                if (dynamic_cast<MonkeyMouseEvent*> (ev)) {
                    iFailedEventCnt++;
                }
            }
            delete ev;    
            
            iTimer->Cancel();
            iTimer->Start(iMonkeyEventInterval*1000, 3000000, TCallBack(&OnTimer, this));
            break;
        }
        i++;
    }

    return i;
}

void CMonkeyTester::FindAndShow()
{
    TApaTaskList tasklist(CMonkeyTester::wsSession);
    TApaTask task(tasklist.FindApp(iUid));
    if (task.Exists()) {
        task.BringToForeground();
    }
}
