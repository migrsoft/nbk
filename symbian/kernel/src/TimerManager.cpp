/*
 ============================================================================
 Name		: TimerManager.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CTimerManager implementation
 ============================================================================
 */

#include "TimerManager.h"
#include "NBrKernel.h"
#include "NBKPlatformData.h"
#include "../../stdc/dom/page.h"

static CTimerManager* l_timerManager = NULL;

// ============================================================================

void NBK_timerCreate(void* pfd, NTimer* timer)
{
    CTimerManager::NewL();
    
    CNbkTimer* t = new (ELeave) CNbkTimer(timer);
    l_timerManager->AddTimer(t);
}

void NBK_timerDelete(void* pfd, NTimer* timer)
{
    l_timerManager->RemoveTimer(timer);
}

void NBK_timerStart(void* pfd, NTimer* timer, int delay, int interval)
{
    CNbkTimer* t = l_timerManager->FindTimer(timer);
    if (t)
        t->iTimer->Start(delay * 1000, interval * 1000, TCallBack(CNbkTimer::OnTimerEvent, t));
}

void NBK_timerStop(void* pfd, NTimer* timer)
{
    CNbkTimer* t = l_timerManager->FindTimer(timer);
    if (t)
        t->iTimer->Cancel();
}

void NBK_callLater(void* pfd, int pageId, TIMER_CALLBACK cb, void* user)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    l_timerManager->SetNbk((CNBrKernel*)p->nbk);
    l_timerManager->CallLater(pageId, (void*)cb, user);
}

// ============================================================================

CNbkTimer::CNbkTimer(NBK_Timer* t)
    : d(t)
{
    iTimer = CPeriodic::NewL(0);
}

CNbkTimer::~CNbkTimer()
{
    iTimer->Cancel();
    delete iTimer;
}

TInt CNbkTimer::OnTimerEvent(TAny* ptr)
{
    CNbkTimer* t = (CNbkTimer*)ptr;
    t->d->func(t->d->user);
    return 0;
}

// ============================================================================

CTimerManager::CTimerManager()
{
    iCallList = (NSList*)sll_create();
    iCallTimer = CPeriodic::NewL(0);
}

CTimerManager::~CTimerManager()
{
    sll_delete(&iCallList);
    iCallTimer->Cancel();
    delete iCallTimer;
    
    iTimerList.Close();
}

CTimerManager* CTimerManager::NewLC()
{
    CTimerManager* self = new (ELeave) CTimerManager();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CTimerManager* CTimerManager::NewL()
{
    if (l_timerManager == NULL) {
        l_timerManager = CTimerManager::NewLC();
        CleanupStack::Pop(); // self;
    }
    return l_timerManager;
}

void CTimerManager::ConstructL()
{
}

void CTimerManager::Destroy()
{
    if (l_timerManager) {
        delete l_timerManager;
        l_timerManager = NULL;
    }
}

void CTimerManager::AddTimer(CNbkTimer* timer)
{
    iTimerList.Append(timer);
    timer->d->id = iTimerList.Count();
}

void CTimerManager::RemoveTimer(NBK_Timer* timer)
{
    for (int i=0; i < iTimerList.Count(); i++) {
        if (iTimerList[i]->d == timer) {
            delete iTimerList[i];
            iTimerList.Remove(i);
            break;
        }
    }
}

CNbkTimer* CTimerManager::FindTimer(NBK_Timer* timer)
{
    for (int i=0; i < iTimerList.Count(); i++) {
        if (iTimerList[i]->d == timer)
            return iTimerList[i];
    }
    return NULL;
}

void CTimerManager::CallLater(int pageId, void* func, void* user)
{
    TCall* c = (TCall*)NBK_malloc0(sizeof(TCall));
    c->pageId = pageId;
    c->func = func;
    c->user = user;
    sll_append(iCallList, c);
    
    if (!iCallTimer->IsActive())
        iCallTimer->Start(10, 5000000, TCallBack(CTimerManager::OnCallEvent, this));
}

TInt CTimerManager::OnCallEvent(TAny* ptr)
{
    CTimerManager* self = (CTimerManager*)ptr;
    self->CallFunc();
    return 0;
}

void CTimerManager::CallFunc()
{
    TCall* c = (TCall*)sll_removeFirst(iCallList);
    NPage* p = (NPage*)iNbk->GetPage();
    
    if (page_getId(p) == c->pageId)
        ((TIMER_CALLBACK)c->func)(c->user);
    else
        smartptr_release(c->user);
    
    NBK_free(c);
    
    iCallTimer->Cancel();
    if (!sll_isEmpty(iCallList))
        iCallTimer->Start(10, 5000000, TCallBack(CTimerManager::OnCallEvent, this));
}
