#ifdef __WIN32__
	#pragma comment(lib, "zlib.lib")
#endif

#include "NbkDll.h"
#include "runtime.h"
#include "LoaderMgr.h"
#include "graphics.h"
#include "ImageMgr.h"
#include "com_migrsoft_nbk_NbkCore.h"
#include "../../stdc/inc/nbk_callback.h"
#include "../../stdc/inc/nbk_cbDef.h"
#include "../../stdc/inc/nbk_gdi.h"
#include "../../stdc/inc/nbk_glue.h"

////////////////////////////////////////////////////////////////////////////////
//
// 来自JAVA的调用
//

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkCoreInit
  (JNIEnv* env, jobject obj)
{
	memory_info_init();
	NbkCore* core = NbkCore::GetInstance();
    core->SetJvm(env);
	core->RegisterJavaMethods(obj);
	//core->SetPageWidth(core->GetScreenWidth());
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkCoreDestroy
  (JNIEnv *, jobject)
{
	NbkCore::Release();
	memory_info();
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkSetPageWidth
  (JNIEnv *, jobject, jint w)
{
	NbkCore* core = NbkCore::GetInstance();
	core->SetPageWidth(w);
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkHandleEvent
  (JNIEnv *, jobject)
{
	NbkCore* core = NbkCore::GetInstance();
	core->HandleEvent();
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkHandleInputEvent
  (JNIEnv *, jobject, jint button, jint stat, jint x, jint y)
{
	NbkCore* core = NbkCore::GetInstance();
	core->HandleInputEvent(button, stat, x, y);
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkHandleCall
  (JNIEnv *, jobject)
{
	NbkCore* core = NbkCore::GetInstance();
	core->Call();
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkLoadUrl
  (JNIEnv* env, jobject, jstring url)
{
	const char* u = env->GetStringUTFChars(url, NULL);
	NbkCore* core = NbkCore::GetInstance();
	core->LoadUrl(u);
	env->ReleaseStringUTFChars(url, u);
}

JNIEXPORT jboolean JNICALL Java_com_migrsoft_nbk_NbkCore_nbkHistoryPrev
  (JNIEnv *, jobject)
{
	NbkCore* core = NbkCore::GetInstance();
	return core->HistoryPrev();
}

JNIEXPORT jboolean JNICALL Java_com_migrsoft_nbk_NbkCore_nbkHistoryNext
  (JNIEnv *, jobject)
{
	NbkCore* core = NbkCore::GetInstance();
	return core->HistoryNext();
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkScrollTo
  (JNIEnv *, jobject, jint x, jint y)
{
	NbkCore* core = NbkCore::GetInstance();
	core->ScrollTo(x, y);
}

////////////////////////////////////////////////////////////////////////////////
//
// 来自NBK的调用
//

void NBK_helper_getViewableRect(void* pfd, NRect* view)
{
	NbkCore* core = (NbkCore*)pfd;
	NRect* vr = core->GetViewRect();
	*view = *vr;
}

void NBK_callLater(void* pfd, int pageId, TIMER_CALLBACK cb, void* user)
{
	NbkCore* core = (NbkCore*)pfd;
	core->CallLater(pageId, cb, user);
}

////////////////////////////////////////////////////////////////////////////////
//
// NbkCore
//

#define PEN_MOVE_MIN	8

NbkCore* NbkCore::mInstance = NULL;

NbkCore* NbkCore::GetInstance()
{
	if (mInstance == NULL) {
		mInstance = new NbkCore();
	}
	return mInstance;
}

void NbkCore::Release()
{
	if (mInstance != NULL) {
		delete mInstance;
		mInstance = NULL;
	}
}

NbkCore::NbkCore()
{
	OBJ_NbkCore = NULL;

	mProbe = probe_create();
#ifdef __ANDROID__
	probe_setPath(mProbe, "/data/data/com.wyl.nshell/nbk_dump.txt");
#endif

	mTimerMgr = new TimerMgr();
	mLoader = new LoaderMgr();
	mGraphic = new Graphic();
	mImageMgr = new ImageMgr();

	Reset();

	NBK_memset(&evtQueue, 0, sizeof(NBK_Msg) * NBK_MAX_EVENT_QUEUE);
	evtAdd = evtUsed = 0;

	mCallLst = sll_create();

	NBK_memset(&mSettings, 0, sizeof(NSettings));
	mSettings.mainFontSize = 16;
	mSettings.fontLevel = NEFNTLV_MIDDLE;
	mSettings.allowImage = N_TRUE;
	mSettings.selfAdaptionLayout = N_FALSE;
	mSettings.initMode = NEREV_STANDARD;
	mSettings.mode = NEREV_TF;

	mHistory = history_create(this);
	history_setSettings(mHistory, &mSettings);
	history_enablePic(mHistory, mSettings.allowImage);

	NPage* page = GetPage();
	page_setEventNotify(page, NbkEventProc, this);
}

NbkCore::~NbkCore()
{
	history_delete(&mHistory);

	sll_delete(&mCallLst);

	delete mImageMgr; mImageMgr = NULL;
	delete mGraphic; mGraphic = NULL;
	delete mLoader; mLoader = NULL;
	delete mTimerMgr; mTimerMgr = NULL;

	probe_delete(&mProbe);

	JNI_env->DeleteGlobalRef(OBJ_NbkCore);
	OBJ_NbkCore = NULL;
}

void NbkCore::Reset()
{
	mFocus = N_NULL;
	point_set(&mLastPos, 0, 0);
	point_set(&mLastInputPos, 0, 0);
	rect_set(&mViewRect, 0, 0, GetScreenWidth(), GetScreenHeight());
}

void NbkCore::RegisterJavaMethods(jobject obj)
{
	OBJ_NbkCore = JNI_env->NewGlobalRef(obj);

	jclass clazz = JNI_env->GetObjectClass(obj);
	MID_getCurrentMillis = JNI_env->GetMethodID(clazz, "getCurrentMillis", "()I");
	MID_scheduleNbkEvent = JNI_env->GetMethodID(clazz, "scheduleNbkEvent", "()V");
	MID_scheduleNbkCall = JNI_env->GetMethodID(clazz, "scheduleNbkCall", "()V");
	MID_notifyState = JNI_env->GetMethodID(clazz, "notifyState", "(III)V");
	MID_updateView = JNI_env->GetMethodID(clazz, "updateView", "(IIII)V");
	MID_getScreenWidth = JNI_env->GetMethodID(clazz, "getScreenWidth", "()I");
	MID_getScreenHeight = JNI_env->GetMethodID(clazz, "getScreenHeight", "()I");
}

void NbkCore::SetPageWidth(coord width)
{
	NPage* page = GetPage();
	page_setScreenWidth(page, width);
}

int NbkCore::EVTQ_INC(int i)
{
	return ((i < NBK_MAX_EVENT_QUEUE - 1) ? i+1 : 0);
}

int NbkCore::EVTQ_DEC(int i)
{
	return ((i > 0) ? i-1 : NBK_MAX_EVENT_QUEUE - 1);
}

void NbkCore::ScheduleEventByJava()
{
	JNI_env->CallVoidMethod(OBJ_NbkCore, MID_scheduleNbkEvent);
}

int NbkCore::NbkEventProc(int id, void* user, void* info)
{
	NbkCore* core = (NbkCore*)user;
	return core->ScheduleEvent(id, info);
}

// 接收内核事件，加入事件队列
int NbkCore::ScheduleEvent(int id, void* info)
{
    NBK_Msg* nevt;

    //printf(" NBK EVENT - %s\n", NBKEventDesc[id]);

    // 调试输出
    if (id == NBK_EVENT_DEBUG1) {
		probe_output(mProbe, info);
        return 0;
    }

    // 附件下载通知
    if (id == NBK_EVENT_DOWNLOAD_FILE) {
        NBK_Event_DownloadFile* evt = (NBK_Event_DownloadFile*)info;
        fprintf(stderr, "Attachment: %s\n", evt->fileName);
        fprintf(stderr, "       Src: %s\n", evt->url);
        fprintf(stderr, "      Size: %d\n", evt->size);
		return 0;
	}

	// 合并事件
    if (   id == NBK_EVENT_DOWNLOAD_IMAGE
        || id == NBK_EVENT_UPDATE_PIC
        || id == NBK_EVENT_GOT_DATA
        || id == NBK_EVENT_REPOSITION )
	{
        // 查找未处理事件
        int pos = EVTQ_DEC(evtAdd);
        nevt = NULL;
        while (evtQueue[pos].eventId && pos != evtAdd) {
            if (evtQueue[pos].eventId == id) {
                nevt = &evtQueue[pos];
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
            return 0;
        }
    }

    // 将事件加入事件队列
    nevt = &evtQueue[evtAdd];
    evtAdd = EVTQ_INC(evtAdd);
    NBK_memset(nevt, 0, sizeof(NBK_Msg));
    nevt->eventId = id;

    switch (id) {
    
    // 准备发出新文档请求
    case NBK_EVENT_NEW_DOC:
    {
        break;
    }

    case NBK_EVENT_NEW_DOC_BEGIN:
	{
		Reset();
		mGraphic->Reset();
        break;
	}

    case NBK_EVENT_HISTORY_DOC:
    {
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

	ScheduleEventByJava();

	return 0;
}

// 处理内核事件
void NbkCore::HandleEvent()
{
    NBK_Msg* nevt;

    nevt = &evtQueue[evtUsed];
    if (nevt->eventId == 0)
        return;

    eventId = nevt->eventId;
    //printf("  DO EVENT - %s\n", NBKEventDesc[eventId]);

    switch (nevt->eventId) {

    // 准备发出新文档请求
    case NBK_EVENT_NEW_DOC:
    {
		NotifyState(NBK_EVENT_NEW_DOC, 0, 0);
        break;
    }

    // 打开历史页面
    case NBK_EVENT_HISTORY_DOC:
    {
		NotifyState(NBK_EVENT_HISTORY_DOC, 0, 0);
        break;
    }

    // Post 数据已发出，等待响应
    case NBK_EVENT_WAITING:
    {
		NotifyState(NBK_EVENT_WAITING, 0, 0);
        break;
    }

    // 接收到数据，开始建立文档
    case NBK_EVENT_NEW_DOC_BEGIN:
    {
		NotifyState(NBK_EVENT_NEW_DOC_BEGIN, 0, 0);
        break;
    }

    // 收到服务器回应
    case NBK_EVENT_GOT_RESPONSE:
    {
		NotifyState(NBK_EVENT_GOT_RESPONSE, 0, 0);
        break;
    }

    // 接收到数据
    case NBK_EVENT_GOT_DATA:
    {
		NotifyState(NBK_EVENT_GOT_DATA, nevt->datReceived, nevt->datTotal);
        break;
    }

    // 更新视图，文档尺寸有改变
    case NBK_EVENT_UPDATE:
    {
		UpdateScreen();
        break;
    }

    // 更新视图，文档尺寸无改变
    case NBK_EVENT_UPDATE_PIC:
    {
		UpdateScreen();
        break;
    }
	
    // 文档排版完成
    case NBK_EVENT_LAYOUT_FINISH:
    {
        mLastPos.x = nevt->x;
        mLastPos.y = nevt->y;
		UpdateScreen();
        break;
    }

    // 正在下载图片
    case NBK_EVENT_DOWNLOAD_IMAGE:
    {
		NotifyState(NBK_EVENT_DOWNLOAD_IMAGE, nevt->imgCur, nevt->imgTotal);
        break;
    }

    // 绘制控件，文档尺寸无改变
    case NBK_EVENT_PAINT_CTRL:
    {
        break;
    }

    // 进入主体模式
    case NBK_EVENT_ENTER_MAINBODY:
    {
        break;
    }

    // 退出主体模式
    case NBK_EVENT_QUIT_MAINBODY:
    {
        break;
    }

    // 退出主体模式
    case NBK_EVENT_QUIT_MAINBODY_AFTER_CLICK:
    {
        break;
    }

    // 通知即将载入错误页
    case NBK_EVENT_LOADING_ERROR_PAGE:
    {
        break;
    }

    // 文档重定位
    case NBK_EVENT_REPOSITION:
    {
		RePosition(nevt->x, nevt->y);
		UpdateScreen();
        break;
	}

    case NBK_EVENT_TEXTSEL_FINISH:
        break;

	// other
    }

    NBK_memset(nevt, 0, sizeof(NBK_Msg));
    evtUsed = EVTQ_INC(evtUsed);
    if (evtQueue[evtUsed].eventId) {
        // 存在未处理事件
		ScheduleEventByJava();
    }
}

void NbkCore::HandleInputEvent(int button, int stat, int x, int y)
{
	switch (button) {
	case 111: // EVENT_MOUSE_LEFT
		switch (stat) {
		case 1: // MOUSE_PRESS
		{
			point_set(&mLastInputPos, x, y);
			NPoint pt = mLastInputPos;
			pt.x += mViewRect.l;
			pt.y += mViewRect.t;
			mFocus = GetFocusByPos(pt);
			SetFocus(mFocus);
			DrawPage();
			break;
		}

		case 2: // MOUSE_MOVE
		{
			coord dx = x - mLastInputPos.x;
			coord dy = y - mLastInputPos.y;
			if (N_ABS(dx) > PEN_MOVE_MIN || N_ABS(dy) > PEN_MOVE_MIN) {
				point_set(&mLastInputPos, x, y);
				if (mFocus)
					mFocus = N_NULL;
				DragPage(-dx, -dy);
			}
			break;
		}

		case 3: // MOUSE_RELEASE
		{
			if (mFocus) {
				NPoint pt;
				point_set(&pt, x, y);
				pt.x += mViewRect.l;
				pt.y += mViewRect.t;
				if (GetFocusByPos(pt) == mFocus)
					ClickFocus(mFocus);
				else
					mFocus = N_NULL;
			}
			DrawPage();
			break;
		}
		} // switch
		break;

	} // switch
}

void NbkCore::DragPage(coord dx, coord dy)
{
	bool update = false;
	NPage* mp = GetPage();
	NSize lcd = {GetScreenWidth(), GetScreenHeight()};
	NSize page = {page_width(mp), page_height(mp)};

	if (page.w > lcd.w) {
		if (dx < 0)
			dx = (mViewRect.l + dx < 0) ? dx - (mViewRect.l + dx) : dx;
		else
			dx = (mViewRect.r + dx > page.w) ? dx - (mViewRect.r + dx - page.w) : dx;
		mViewRect.l += dx;
		mViewRect.r += dx;
		update = true;
	}

	if (page.h > lcd.h) {
		if (dy < 0)
			dy = (mViewRect.t + dy < 0) ? dy - (mViewRect.t + dy) : dy;
		else
			dy = (mViewRect.b + dy > page.h) ? dy - (mViewRect.b + dy - page.h) : dy;
		mViewRect.t += dy;
		mViewRect.b += dy;
		update = true;
	}

	if (update)
		DrawPage();
}

void NbkCore::RePosition(coord x, coord y)
{
	NPage* mp = GetPage();
	NSize lcd = {GetScreenWidth(), GetScreenHeight()};
	NSize page = {page_width(mp), page_height(mp)};

	if (lcd.h >= page.h)
        return;

	if (y + lcd.h > page.h)
        y = page.h - lcd.h;
    mViewRect.t = y;
    mViewRect.b = y + lcd.h;
}

int NbkCore::GetCurrentMillis()
{
	return JNI_env->CallIntMethod(OBJ_NbkCore, MID_getCurrentMillis);
}

void NbkCore::LoadUrl(const char* url)
{
	NPage* page = history_curr(mHistory);
	page_loadUrl(page, url);
}

bool NbkCore::HistoryPrev()
{
	return (bool)history_prev(mHistory);
}

bool NbkCore::HistoryNext()
{
	return (bool)history_next(mHistory);
}

coord NbkCore::GetScreenWidth()
{
	if (OBJ_NbkCore)
		return (coord)JNI_env->CallIntMethod(OBJ_NbkCore, MID_getScreenWidth);
	else
		return 0;
}

coord NbkCore::GetScreenHeight()
{
	if (OBJ_NbkCore)
		return (coord)JNI_env->CallIntMethod(OBJ_NbkCore, MID_getScreenHeight);
	else
		return 0;
}

NPage* NbkCore::GetPage()
{
	return history_curr(mHistory);
}

void NbkCore::UpdateViewRect()
{
	NPage* mp = GetPage();
	NSize lcd = {GetScreenWidth(), GetScreenHeight()};
	NSize page = {page_width(mp), page_height(mp)};

	mViewRect.l = 0;
	mViewRect.r = N_MIN(lcd.w, page.w);
	mViewRect.t = 0;
	mViewRect.b = N_MIN(lcd.h, page.h);

    if (mLastPos.x) {
        coord vw = rect_getWidth(&mViewRect);
        if (page.w > lcd.w && mViewRect.l + mLastPos.x < page.w) {
            mViewRect.l += mLastPos.x;
            if (mViewRect.l + vw > page.w)
                mViewRect.l = page.w - vw;
            mViewRect.r = mViewRect.l + vw;
        }
    }
    if (mLastPos.y) {
        coord vh = rect_getHeight(&mViewRect);
        if (page.h > lcd.h && mViewRect.t + mLastPos.y < page.h) {
            mViewRect.t += mLastPos.y;
            if (mViewRect.t + vh > page.h)
                mViewRect.t = page.h - vh;
            mViewRect.b = mViewRect.t + vh;
        }
    }
}

void NbkCore::DrawPage()
{
	NPage* page = GetPage();
	page_paint(page, &mViewRect);

	point_set(&mLastPos, mViewRect.l, mViewRect.t);

	int w = page_width(page);
	int h = page_height(page);
	JNI_env->CallVoidMethod(OBJ_NbkCore, MID_updateView, w, h, mViewRect.l, mViewRect.t);
}

void NbkCore::UpdateScreen()
{
    if (   eventId != NBK_EVENT_UPDATE_PIC
        && eventId != NBK_EVENT_PAINT_CTRL
        && eventId != NBK_EVENT_REPOSITION ) {
		UpdateViewRect();
	}

	if (eventId == NBK_EVENT_PAINT_CTRL) {
	}
	else {
		DrawPage();
	}
}

NNode* NbkCore::GetFocusByPos(NPoint pos)
{
	NPage* page = GetPage();
	return doc_getFocusNodeByPos(page->doc, pos.x, pos.y);
}

void NbkCore::SetFocus(NNode* node)
{
	NPage* page = GetPage();
	page_setFocusedNode(page, node);
}

void NbkCore::ClickFocus(NNode* node)
{
	if (node) {
		NPage* page = GetPage();
		doc_clickFocusNode(page->doc, node);
	}
}

void NbkCore::ScrollTo(int x, int y)
{
	rect_move(&mViewRect, x, y);
	DrawPage();
}

void NbkCore::NotifyState(int state, int v1, int v2)
{
	JNI_env->CallVoidMethod(OBJ_NbkCore, MID_notifyState, state, v1, v2);
}

void NbkCore::CallLater(int pageId, TIMER_CALLBACK cb, void* user)
{
	NBK_Call* c = (NBK_Call*)NBK_malloc(sizeof(NBK_Call));
	c->pageId = pageId;
	c->func = cb;
	c->user = user;
	sll_append(mCallLst, c);

	JNI_env->CallVoidMethod(OBJ_NbkCore, MID_scheduleNbkCall);
}

void NbkCore::Call()
{
	NBK_Call* c = (NBK_Call*)sll_removeFirst(mCallLst);

	if (c == N_NULL)
		return;

	NPage* page = GetPage();
	// 当异步调用时，需要确保页面有效性
	if (page_getId(page) == c->pageId) {
		((TIMER_CALLBACK)c->func)(c->user);
	}
	else {
		smartptr_release(c->user);
	}

	NBK_free(c);
}
