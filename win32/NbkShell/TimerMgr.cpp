#include "stdafx.h"
#include "TimerMgr.h"
#include "NbkCore.h"
#include "NbkShell.h"
#include "NbkEvent.h"
#include "../../stdc/dom/page.h"
#include "../../stdc/tools/smartPtr.h"

////////////////////////////////////////////////////////////////////////////////
//
// NBK 定时器调用
//

void NBK_timerCreate(void* pfd, NTimer* timer)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Create(timer);
}

void NBK_timerDelete(void* pfd, NTimer* timer)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Delete(timer);
}

void NBK_timerStart(void* pfd, NTimer* timer, int delay, int interval)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Start(timer, delay, interval);
}

void NBK_timerStop(void* pfd, NTimer* timer)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Stop(timer);
}

void NBK_callLater(void* pfd, int pageId, TIMER_CALLBACK cb, void* user)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->CallLater(pageId, cb, user);
}

////////////////////////////////////////////////////////////////////////////////
//
// TimerMgr 接口类
//

TimerMgr::TimerMgr(NbkCore* core)
{
	mShell = NULL;
	mCore = core;

    mAllocStart = 0;
	NBK_memset(mTimers, 0, sizeof(NTimer*) * MAX_TIMERS);
}

TimerMgr::~TimerMgr()
{
}

void TimerMgr::Create(NTimer* timer)
{
	timer->id = 0;

    // 从上次分配的下一地址分配定时器，避免同一地址立即被不同定时器使用

	for (int j = 0; j < 2; j++) {
		for (int i = mAllocStart; i < MAX_TIMERS; i++) {
			if (mTimers[i] == NULL) {
				mTimers[i] = timer;
				timer->id = ++i; // 从 1 开始
				mAllocStart = (i == MAX_TIMERS) ? 0 : i;
				return;
			}
		}
		mAllocStart = 0;
	}
}

void TimerMgr::Delete(NTimer* timer)
{
    if (timer->id > 0) {
        mTimers[timer->id - 1] = NULL;
    }
}

void TimerMgr::Start(NTimer* timer, int delay, int interval)
{
    //printf("timer %d start\n", timer->id);
    if (timer->id > 0) {
		mShell->StartTimer(timer->id, interval);
    }
}

void TimerMgr::Stop(NTimer* timer)
{
    //printf("timer %d stop\n", timer->id);
    if (timer->id > 0)
		mShell->StopTimer(timer->id);
}

void TimerMgr::OnTimer(int id)
{
	NTimer* t = mTimers[id - 1];
	if (t->run)
		t->func(t->user);
}

void TimerMgr::CallLater(int pageId, TIMER_CALLBACK cb, void* user)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_NBK_CALL);
	e->SetCallback(pageId, (void*)cb, user);
	mCore->PostEvent(e);
}

void TimerMgr::HandleEvent(NbkEvent* evt)
{
	switch (evt->GetId()) {
	case NbkEvent::EVENT_NBK_CALL:
	{
		void* func;
		void* user;
		int pageId;
		evt->GetCallback(&pageId, &func, &user);
	
		NPage* page = mCore->GetPage();

		// 当异步调用时，需要确保页面有效性
		if (page_getId(page) == pageId) {
			((TIMER_CALLBACK)func)(user);
		}
		else {
			smartptr_release(user);
		}
		break;
	}
	}
}
