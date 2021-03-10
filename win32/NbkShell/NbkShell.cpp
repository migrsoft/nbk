#include "stdafx.h"
#include "NbkShell.h"
#include "MWinMgr.h"
#include "NbkCore.h"
#include "NbkEvent.h"
#include "../NbkCore/nbk_mem.h"

NbkShell::NbkShell() : MWin()
{
	wcscpy_s(mClassName, _T("NBK_SHELL"));

	mScreenWidth = 640;
	mScreenHeight = 600;

	mPageWidth = mPageHeight = 1;

	mCanvas = NULL;
	mScreen = NULL;
	mGdi = NULL;

	mCore = NULL;
	mCoreThread = NULL;
}

NbkShell::~NbkShell()
{
	if (mCoreThread) {
		WaitForSingleObject(mCoreThread, INFINITE);
	}

	if (mCore) {
		delete mCore;
		mCore = NULL;
	}

	if (mCanvas)
		delete mCanvas;
	if (mScreen) {
		delete mScreen;
		mScreen = NULL;
	}
	if (mGdi) {
		delete mGdi;
		mGdi = NULL;
	}

	OutputDebugString(_T("NbkShell Over\n"));

	memory_info();
	NbkEvent::MemoryInfo();
}

bool NbkShell::Create(HWND parent)
{
	HINSTANCE hInstance = MWinMgr::GetInstance()->GetAppInstance();

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MWinMgr::MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= mClassName;
	wcex.hIconSm		= NULL;

	if (!RegisterClassEx(&wcex))
		return false;

	mHwnd = CreateWindow(mClassName, NULL, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
		0, 0, mScreenWidth, mScreenHeight, parent, NULL, hInstance, this);

	if (!mHwnd)
		return false;

	RECT r;
	GetClientRect(mHwnd, &r);
	mScreenWidth = r.right - r.left;
	mScreenHeight = r.bottom - r.top;

	UpdateScrollbar(FALSE);

	mCanvas = new Bitmap(mScreenWidth, mScreenHeight);
	mScreen = new Bitmap(mScreenWidth, mScreenHeight);
	mGdi = new Graphics(mCanvas);

	mCore = new NbkCore(this);
	mCoreThread = CreateThread(NULL, 0, NbkCore::Proc, mCore, 0, NULL);
	mCore->PostEvent(new NbkEvent(NbkEvent::EVENT_INIT));

	return true;
}

void NbkShell::OnCreate()
{
}

void NbkShell::OnDestroy()
{
	mCore->PostEvent(new NbkEvent(NbkEvent::EVENT_QUIT));
}

bool NbkShell::OnPaint()
{
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(mHwnd, &ps);
	Graphics g(dc);
	g.DrawImage(mScreen, 0, 0);
	EndPaint(mHwnd, &ps);
	return true;
}

void NbkShell::PostEvent(NbkEvent* evt)
{
	PostMessage(mHwnd, WM_USER, (WPARAM)evt, NULL);
}

void NbkShell::OnUser(WPARAM wParam, LPARAM lParam)
{
	NbkEvent* e = (NbkEvent*)wParam;

	switch (e->GetId()) {
	case NbkEvent::EVENT_NEW_DOC_BEGIN:
		OnNewDocumentBegin();
		break;

	case NbkEvent::EVENT_UPDATE:
		OnPageUpdate();
		break;

	case NbkEvent::EVENT_PAGE_SIZE:
		OnPageSizeChanged(e->GetPageWidth(), e->GetPageHeight());
		break;
	}

	delete e;
}

void NbkShell::StartTimer(int id, int interval)
{
	SetTimer(mHwnd, id, interval, NULL);
}

void NbkShell::StopTimer(int id)
{
	KillTimer(mHwnd, id);
}

void NbkShell::OnTimer(WPARAM wParam)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_TIMER);
	e->SetTimerId((int)wParam);
	mCore->PostEvent(e);
}

void NbkShell::LoadUrl(const char* url, bool del)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_LOAD_URL);
	e->SetUrl(url, del);
	mCore->PostEvent(e);
}

void NbkShell::LoadPrevPage()
{
	mCore->PostEvent(new NbkEvent(NbkEvent::EVENT_LOAD_PREV_PAGE));
}

void NbkShell::LoadNextPage()
{
	mCore->PostEvent(new NbkEvent(NbkEvent::EVENT_LOAD_NEXT_PAGE));
}

void NbkShell::OnPageSizeChanged(int w, int h)
{
	mPageWidth = w;
	mPageHeight = h;
	UpdateScrollbar(TRUE);
}

void NbkShell::CalcScrollInfo(LPSCROLLINFO si, int view, int extent)
{
	if (extent <= view) {
		si->nMax = 0;
		si->nMin = 0;
		si->nPage = 0;
		si->nPos = 0;
	}
	else {
		si->nMax = extent;
		si->nMin = 0;
		si->nPage = view;
		si->nPos = 0;
	}
}

void NbkShell::UpdateScrollbar(BOOL redraw)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

	CalcScrollInfo(&si, mScreenHeight, mPageHeight);
	SetScrollInfo(mHwnd, SB_VERT, &si, redraw);

	CalcScrollInfo(&si, mScreenWidth, mPageWidth);
	SetScrollInfo(mHwnd, SB_HORZ, &si, redraw);
}

void NbkShell::OnVertScroll(int value, int pos)
{
	switch (value) {
	case SB_THUMBPOSITION:
		SetScrollPos(mHwnd, SB_VERT, pos, TRUE);
		ScrollPage();
		break;

	case SB_PAGEUP:
		OnScrollByPage(SB_VERT, false);
		break;

	case SB_PAGEDOWN:
		OnScrollByPage(SB_VERT, true);
		break;

	case SB_LINEUP:
		break;

	case SB_LINEDOWN:
		break;
	}
}

void NbkShell::OnHorzScroll(int value, int pos)
{
}

void NbkShell::ScrollPage()
{
	int x = GetScrollPos(mHwnd, SB_HORZ);
	int y = GetScrollPos(mHwnd, SB_VERT);
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_SCROLL_TO);
	e->SetX(x);
	e->SetY(y);
	mCore->PostEvent(e);
}

bool NbkShell::OnLeftButton(bool down, int x, int y)
{
	NbkEvent* e = new NbkEvent(NbkEvent::EVENT_LEFT_BUTTON);
	e->SetX(x);
	e->SetY(y);
	e->SetState((down) ? NbkEvent::STAT_DOWN : NbkEvent::STAT_UP);
	mCore->PostEvent(e);
	return true;
}

void NbkShell::ShowPicture(bool show)
{
	mCore->ShowPicture(show);
}

void NbkShell::OnPageUpdate()
{
	Graphics g(mScreen);
	g.DrawImage(mCanvas, 0, 0);
	InvalidateRect(mHwnd, NULL, FALSE);
}

void NbkShell::OnScrollByPage(int sbar, bool down)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS;

	GetScrollInfo(mHwnd, sbar, &si);

	int pos = (down) ? si.nPos + si.nPage : si.nPos - si.nPage;
	SetScrollPos(mHwnd, sbar, pos, TRUE);

	ScrollPage();
}

void NbkShell::OnNewDocumentBegin()
{
	Graphics g(mCanvas);
	SolidBrush b(Color(255, 255, 255, 255));
	g.FillRectangle(&b, 0, 0, mScreenWidth, mScreenHeight);
	OnPageUpdate();

	mPageWidth = mPageHeight = 1;
	UpdateScrollbar(TRUE);
}
