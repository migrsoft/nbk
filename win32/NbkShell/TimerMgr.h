#pragma once

#include "../../stdc/inc/nbk_timer.h"

class NbkShell;
class NbkCore;
class NbkEvent;

class TimerMgr {
	
private:
	static const int MAX_TIMERS = 64;

	NbkShell*	mShell;
	NbkCore*	mCore;

    int     mAllocStart;
	NTimer* mTimers[MAX_TIMERS];

public:
	TimerMgr(NbkCore* core);
	~TimerMgr();

	void SetShell(NbkShell* shell) { mShell = shell; }

	void Create(NTimer* timer);
	void Delete(NTimer* timer);
	void Start(NTimer* timer, int delay, int interval);
	void Stop(NTimer* timer);

	void OnTimer(int id);

	void CallLater(int pageId, TIMER_CALLBACK cb, void* user);
	void HandleEvent(NbkEvent* evt);
};
