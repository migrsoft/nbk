#include "stdafx.h"
#include "NbkCore.h"
#include "NbkShell.h"
#include "NbkEvent.h"
#include "TimerMgr.h"
#include "LoaderMgr.h"
#include "Graphic.h"
#include "ImageMgr.h"
#include "../../stdc/inc/nbk_gdi.h"
#include "../../stdc/inc/nbk_cbDef.h"

////////////////////////////////////////////////////////////////////////////////
//
// 来自NBK的调用
//

void NBK_helper_getViewableRect(void* pfd, NRect* view)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetViewableRect(view);
}

NbkCore::NbkCore(NbkShell* shell)
{
	mShell = shell;

	mTimerMgr = new TimerMgr(this);
	mTimerMgr->SetShell(shell);
	mLoader = new LoaderMgr(this);
	mGraphic = new GraphicsW32();
	mGraphic->SetShell(shell);
	mImageMgr = new ImageMgr(this);

	mProbe = NULL;
	mHistory = NULL;

	mEventQueue = sll_create();

	InitializeCriticalSection(&mEventCS);
	mEVENT = CreateEvent(NULL, FALSE, FALSE, NULL);
}

NbkCore::~NbkCore()
{
	CloseHandle(mEVENT);
	DeleteCriticalSection(&mEventCS);

	NbkEvent* e = (NbkEvent*)sll_first(mEventQueue);
	while (e) {
		delete e;
		e = (NbkEvent*)sll_next(mEventQueue);
	}
	sll_delete(&mEventQueue);

	delete mImageMgr;
	delete mGraphic;
	delete mLoader;
	delete mTimerMgr;

	OutputDebugString(_T("NbkCore Over\n"));
}

DWORD WINAPI NbkCore::Proc(LPVOID pParam)
{
	NbkCore* core = (NbkCore*)pParam;
	return core->EventLoop();
}

DWORD NbkCore::EventLoop()
{
	BOOL run = TRUE;
	NbkEvent* e;
	//TCHAR dbg[64];

	while (run) {
		WaitForSingleObject(mEVENT, INFINITE);

		while (run) {
			EnterCriticalSection(&mEventCS);
			e = (NbkEvent*)sll_removeFirst(mEventQueue);
			LeaveCriticalSection(&mEventCS);

			if (e == NULL)
				break;

			//wsprintf(dbg, _T("Event %d\n"), e->GetId());
			//OutputDebugString(dbg);

			switch (e->GetId()) {
			case NbkEvent::EVENT_INIT:
				Init();
				break;

			case NbkEvent::EVENT_QUIT:
				Quit();
				run = FALSE;
				break;

			case NbkEvent::EVENT_TIMER:
				mTimerMgr->OnTimer(e->GetTimerId());
				break;

			case NbkEvent::EVENT_NBK:
				HandleEvent();
				break;

			case NbkEvent::EVENT_NBK_CALL:
				mTimerMgr->HandleEvent(e);
				break;

			case NbkEvent::EVENT_LOAD_URL:
				LoadUrl(e->GetUrl());
				break;

			case NbkEvent::EVENT_LOAD_PREV_PAGE:
				LoadPrevPage();
				break;

			case NbkEvent::EVENT_LOAD_NEXT_PAGE:
				LoadNextPage();
				break;

			case NbkEvent::EVENT_RECE_HEADER:
			case NbkEvent::EVENT_RECE_DATA:
			case NbkEvent::EVENT_RECE_COMPLETE:
			case NbkEvent::EVENT_RECE_ERROR:
				mLoader->HandleEvent(e);
				break;

			case NbkEvent::EVENT_DECODE_IMG:
				mImageMgr->HandleEvent(e);
				break;

			case NbkEvent::EVENT_SCROLL_TO:
				ScrollTo(e->GetX(), e->GetY());
				break;

			case NbkEvent::EVENT_LEFT_BUTTON:
				HandleInputEvent(e->GetId(), e->GetState(), e->GetX(), e->GetY());
				break;
			}

			delete e;
		}
	}

	OutputDebugString(_T("Core-Event-Loop-Over\n"));
	return 0;
}

void NbkCore::Init()
{
	mProbe = probe_create();

	Reset();

	NBK_memset(&evtQueue, 0, sizeof(NBK_Msg) * NBK_MAX_EVENT_QUEUE);
	evtAdd = evtUsed = 0;

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
	page_setScreenWidth(page, GetScreenWidth());

	mLoader->Init();
}

void NbkCore::Quit()
{
	mLoader->Quit();

	history_delete(&mHistory);
	probe_delete(&mProbe);
}

void NbkCore::PostEvent(NbkEvent* evt)
{
	//TCHAR msg[32];
	//wsprintf(msg, _T("post %d\n"), evt->GetId());
	//OutputDebugString(msg);

	EnterCriticalSection(&mEventCS);
	sll_append(mEventQueue, evt);
	LeaveCriticalSection(&mEventCS);

	SetEvent(mEVENT);
}

NPage* NbkCore::GetPage()
{
	return history_curr(mHistory);
}

int NbkCore::NbkEventProc(int id, void* user, void* info)
{
	NbkCore* core = (NbkCore*)user;
	return core->ScheduleEvent(id, info);
}

int NbkCore::EVTQ_INC(int i)
{
	return ((i < NBK_MAX_EVENT_QUEUE - 1) ? i+1 : 0);
}

int NbkCore::EVTQ_DEC(int i)
{
	return ((i > 0) ? i-1 : NBK_MAX_EVENT_QUEUE - 1);
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

	PostEvent(new NbkEvent(NbkEvent::EVENT_NBK));

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
        break;
    }

    // 打开历史页面
    case NBK_EVENT_HISTORY_DOC:
    {
        break;
    }

    // Post 数据已发出，等待响应
    case NBK_EVENT_WAITING:
    {
        break;
    }

    // 接收到数据，开始建立文档
    case NBK_EVENT_NEW_DOC_BEGIN:
    {
		mShell->PostEvent(new NbkEvent(NbkEvent::EVENT_NEW_DOC_BEGIN));
        break;
    }

    // 收到服务器回应
    case NBK_EVENT_GOT_RESPONSE:
    {
        break;
    }

    // 接收到数据
    case NBK_EVENT_GOT_DATA:
    {
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
        break;
    }

    // 绘制控件，文档尺寸无改变
    case NBK_EVENT_PAINT_CTRL:
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
        break;
	}

    case NBK_EVENT_TEXTSEL_FINISH:
        break;

    } // switch

    NBK_memset(nevt, 0, sizeof(NBK_Msg));
    evtUsed = EVTQ_INC(evtUsed);
    if (evtQueue[evtUsed].eventId) {
        // 存在未处理事件
		PostEvent(new NbkEvent(NbkEvent::EVENT_NBK));
    }
}

void NbkCore::HandleInputEvent(int button, int stat, int x, int y)
{
	switch (button) {
	case NbkEvent::EVENT_LEFT_BUTTON:
		switch (stat) {
		case NbkEvent::STAT_DOWN:
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

		case NbkEvent::STAT_UP:
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
		}
		break;
	}
}

void NbkCore::UpdateViewRect()
{
	coord w = GetScreenWidth();
	coord h = GetScreenHeight();
	NPage* page = GetPage();
	coord pw = page_width(page);
	coord ph = page_height(page);

	mViewRect.l = 0;
	mViewRect.r = N_MIN(w, pw);
	mViewRect.t = 0;
	mViewRect.b = N_MIN(h, ph);

	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_PAGE_SIZE);
	e->SetPageSize(pw, ph);
	mShell->PostEvent(e);
}

void NbkCore::DrawPage()
{
	NPage* page = GetPage();
	page_paint(page, &mViewRect);

	mLastPos.x = mViewRect.l;
	mLastPos.y = mViewRect.t;

	int w = page_width(page);
	int h = page_height(page);

	mShell->PostEvent(new NbkEvent(NbkEvent::EVENT_UPDATE));
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

void NbkCore::Reset()
{
	mFocus = N_NULL;
	point_set(&mLastPos, 0, 0);
	point_set(&mLastInputPos, 0, 0);
	rect_set(&mViewRect, 0, 0, GetScreenWidth(), GetScreenHeight());
}

coord NbkCore::GetScreenWidth()
{
	return (coord)mShell->GetScreenWidth();
}

coord NbkCore::GetScreenHeight()
{
	return (coord)mShell->GetScreenHeight();
}

void NbkCore::LoadUrl(const char* url)
{
	NPage* page = GetPage();
	page_loadUrl(page, url);
}

void NbkCore::LoadPrevPage()
{
	mImageMgr->StopAll();
	history_prev(mHistory);
}

void NbkCore::LoadNextPage()
{
	mImageMgr->StopAll();
	history_next(mHistory);
}

void NbkCore::GetViewableRect(NRect* r)
{
	r->l = mViewRect.l;
	r->t = mViewRect.t;
	r->r = r->l + GetScreenWidth();
	r->b = r->t + GetScreenHeight();
}

void NbkCore::ShowPicture(bool show)
{
	mSettings.allowImage = (show) ? N_TRUE : N_FALSE;
}
