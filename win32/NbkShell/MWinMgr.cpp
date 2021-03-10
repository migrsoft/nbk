#include "stdafx.h"
#include "MWinMgr.h"
#include "MWin.h"

MWinMgr* MWinMgr::mInstance = NULL;

void MWinMgr::Init(HINSTANCE hInstance)
{
	if (mInstance == NULL) {
		mInstance = new MWinMgr(hInstance);
	}
}

void MWinMgr::Quit()
{
	if (mInstance) {
		delete mInstance;
		mInstance = NULL;
	}
}

MWinMgr* MWinMgr::GetInstance()
{
	return mInstance;
}

// 全局窗口过程
LRESULT CALLBACK MWinMgr::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MWin* w = NULL;

	if (message == WM_CREATE) {
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		if (cs) {
			w = (MWin*)cs->lpCreateParams;
			if (w) {
				w->SetHwnd(hWnd);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)w);
			}
		}
	}

	w = dynamic_cast<MWin*>((MWin*)GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (w) {
		switch (message) {
		case WM_CREATE:
			w->OnCreate();
			return 0;

		case WM_DESTROY:
			w->OnDestroy();
			return 0;

		case WM_PAINT:
			if (w->OnPaint())
				return 0;
			break;

		case WM_COMMAND:
		{
			int wmId    = LOWORD(wParam);
			int wmEvent = HIWORD(wParam);
			if (w->OnCommand(wmId, wmEvent))
				return 0;
			break;
		}

		case WM_TIMER:
			w->OnTimer(wParam);
			return 0;

		case WM_KEYDOWN:
			if (w->OnKeyDown(wParam, lParam))
				return 0;
			break;

		case WM_KEYUP:
			if (w->OnKeyUp(wParam, lParam))
				return 0;
			break;

		case WM_LBUTTONDOWN:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			if (w->OnLeftButton(true, x, y))
				return 0;
			break;
		}

		case WM_LBUTTONUP:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			if (w->OnLeftButton(false, x, y))
				return 0;
			break;
		}

		case WM_VSCROLL:
		{
			int value = LOWORD(wParam);
			int pos = HIWORD(wParam);
			w->OnVertScroll(value, pos);
			return 0;
		}

		case WM_HSCROLL:
		{
			int value = LOWORD(wParam);
			int pos = HIWORD(wParam);
			w->OnHorzScroll(value, pos);
			return 0;
		}

		default:
			if (message >= WM_USER) {
				w->OnUser(wParam, lParam);
				return 0;
			}
		}

		return w->WndProc(message, wParam, lParam);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

MWinMgr::MWinMgr(HINSTANCE hInstance)
{
	mAppInstance = hInstance;
	mAccelTable = NULL;

	mMainFont = NULL;
}

MWinMgr::~MWinMgr()
{
	if (mMainFont)
		DeleteObject(mMainFont);

	OutputDebugString(_T("Windows Manager Quit\n"));
}

HINSTANCE MWinMgr::GetAppInstance()
{
	return mAppInstance;
}

void MWinMgr::LoadAccelerators(int resId)
{
	mAccelTable = ::LoadAccelerators(mAppInstance, MAKEINTRESOURCE(resId));
}

int MWinMgr::MainLoop()
{
	MSG	msg;

	while (GetMessage(&msg, NULL, 0, 0)) {

		if (mAccelTable && TranslateAccelerator(msg.hwnd, mAccelTable, &msg))
			continue;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

HFONT MWinMgr::GetMainFont()
{
	if (mMainFont == NULL) {
		mMainFont = CreateFont(
			20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET,
			OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			VARIABLE_PITCH | FF_DONTCARE, L"\u5fae\u8f6f\u96c5\u9ed1");
	}

	return mMainFont;
}
