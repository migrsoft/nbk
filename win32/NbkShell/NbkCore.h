#pragma once

#include "../../stdc/inc/nbk_settings.h"
#include "../../stdc/inc/nbk_gdi.h"
#include "../../stdc/dom/history.h"
#include "../../stdc/tools/slist.h"
#include "../../stdc/tools/probe.h"

class NbkShell;
class NbkEvent;
class TimerMgr;
class LoaderMgr;
class GraphicsW32;
class ImageMgr;

class NbkCore {

private:
	NbkShell*	mShell;

	NSList*		mEventQueue;

	CRITICAL_SECTION	mEventCS;
	HANDLE				mEVENT;

private:
	static const int NBK_MAX_EVENT_QUEUE = 64;

	typedef struct _NBK_Msg {
        
		int eventId;
        
		int imgCur;
		int imgTotal;
		int datReceived;
		int datTotal;
        
		int x;
		int y;
        
		float	zoom;

	} NBK_Msg;

    // 内核事件队列
    NBK_Msg		evtQueue[NBK_MAX_EVENT_QUEUE];
    int			evtAdd;
    int			evtUsed;
    int			eventId;

	int EVTQ_INC(int index);
	int EVTQ_DEC(int index);

private:
	TimerMgr*		mTimerMgr;
	LoaderMgr*		mLoader;
	GraphicsW32*	mGraphic;
	ImageMgr*		mImageMgr;

	NSettings	mSettings;
	NHistory*	mHistory;
	NBK_probe*	mProbe;

	NRect		mViewRect;
	NPoint		mLastPos;
	NPoint		mLastInputPos;
	NNode*		mFocus;

public:
	NbkCore(NbkShell* shell);
	~NbkCore();

	static DWORD WINAPI Proc(LPVOID pParam);

	void PostEvent(NbkEvent* evt);

	NbkShell* GetShell() { return mShell; }
	TimerMgr* GetTimerMgr() { return mTimerMgr; }
	LoaderMgr* GetLoader() { return mLoader; }
	GraphicsW32* GetGraphic() { return mGraphic; }
	ImageMgr* GetImageMgr() { return mImageMgr; }

	NPage* GetPage();

	void GetViewableRect(NRect* r);

	void ShowPicture(bool show);

private:
	DWORD EventLoop();

	void Init();
	void Quit();

	void Reset();

	static int NbkEventProc(int id, void* user, void* info);
	int ScheduleEvent(int id, void* info);
	void HandleEvent();
	void HandleInputEvent(int button, int stat, int x, int y);

	void UpdateViewRect();
	void DrawPage();
	void UpdateScreen();

	NNode* GetFocusByPos(NPoint pos);
	void SetFocus(NNode* node);
	void ClickFocus(NNode* node);

	void ScrollTo(int x, int y);

	coord GetScreenWidth();
	coord GetScreenHeight();

	void LoadUrl(const char* url);
	void LoadPrevPage();
	void LoadNextPage();
};
