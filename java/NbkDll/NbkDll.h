#pragma once

#ifdef __WIN32__
	#include "win32/jni.h"
#endif
#ifdef __ANDROID__
	#include <jni.h>
#endif
#include "../../stdc/inc/nbk_settings.h"
#include "../../stdc/inc/nbk_timer.h"
#include "../../stdc/dom/history.h"
#include "../../stdc/tools/probe.h"
#include "../../stdc/tools/slist.h"

class TimerMgr;
class LoaderMgr;
class Graphic;
class ImageMgr;

class NbkCore {

private:
	JNIEnv*		JNI_env;
	jweak		OBJ_NbkCore;

	jmethodID	MID_getCurrentMillis;

	jmethodID	MID_scheduleNbkEvent;
	jmethodID	MID_scheduleNbkCall;

	jmethodID	MID_notifyState;
	jmethodID	MID_updateView;
	jmethodID	MID_getScreenWidth;
	jmethodID	MID_getScreenHeight;

private:
	static NbkCore*	mInstance;

	NSettings	mSettings;
	NHistory*	mHistory;

	TimerMgr*	mTimerMgr;
	LoaderMgr*	mLoader;
	Graphic*	mGraphic;
	ImageMgr*	mImageMgr;

	NBK_probe*	mProbe;

public:
	static NbkCore* GetInstance();
	static void Release();

	NbkCore();
	~NbkCore();

	void SetJvm(JNIEnv* env) { JNI_env = env; }
	JNIEnv* GetJvm() { return JNI_env; }

	void RegisterJavaMethods(jobject obj);

	TimerMgr* GetTimerMgr() { return mTimerMgr; }
	LoaderMgr* GetLoader() { return mLoader; }
	Graphic* GetGraphic() { return mGraphic; }
	ImageMgr* GetImageMgr() { return mImageMgr; }

	void SetPageWidth(coord width);

	int ScheduleEvent(int id, void* info);
	void HandleEvent();
	void HandleInputEvent(int button, int stat, int x, int y);

	void CallLater(int pageId, TIMER_CALLBACK cb, void* user);
	void Call();

	int GetCurrentMillis();

	void LoadUrl(const char* url);

	bool HistoryPrev();
	bool HistoryNext();

	coord GetScreenWidth();
	coord GetScreenHeight();

	NRect* GetViewRect() { return &mViewRect; }

	void ScrollTo(int x, int y);

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

	// 调用表
	typedef struct _NBK_Call {
		int				pageId;
		TIMER_CALLBACK	func;
		void*			user;
	} NBK_Call;

	NSList*		mCallLst;

	/////

	NRect		mViewRect;
	NPoint		mLastPos;
	NPoint		mLastInputPos;
	NNode*		mFocus;

private:
	static int NbkEventProc(int id, void* user, void* info);

	void ScheduleEventByJava();

	void Reset();

	NPage* GetPage();

	void UpdateViewRect();
	void DrawPage();
	void UpdateScreen();

	NNode* GetFocusByPos(NPoint pos);
	void SetFocus(NNode* node);
	void ClickFocus(NNode* node);

	void DragPage(coord dx, coord dy);
	void RePosition(coord x, coord y);

	void NotifyState(int state, int v1, int v2);
};
