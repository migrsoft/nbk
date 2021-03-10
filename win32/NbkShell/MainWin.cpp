#include "stdafx.h"
#include "Resource.h"
#include "MainWin.h"
#include "MWinMgr.h"
#include "MEdit.h"
#include "NbkShell.h"
#include "../../stdc/tools/unicode.h"
#include "../../stdc/tools/str.h"

void MainWin::EditListener::OnEnter(int id, bool down)
{
	if (id == IDC_ADDREDIT && down)
		mClient->LoadUrlFromAddress();
}

MainWin::MainWin()
{
	mAddressEdit = NULL;
	mShell = NULL;

	mEditListener = new EditListener(this);

	mShowPic = true;
}

MainWin::~MainWin()
{
	if (mAddressEdit)
		delete mAddressEdit;
	if (mGoPrev)
		delete mGoPrev;
	if (mGoNext)
		delete mGoNext;

	if (mShell)
		delete mShell;

	delete mEditListener;

	OutputDebugString(_T("MainWin Over\n"));
}

bool MainWin::Create(HWND parent)
{
	MWinMgr* winMgr = MWinMgr::GetInstance();

	HINSTANCE hInstance = winMgr->GetAppInstance();

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_NBKSHELL, szWindowClass, MAX_LOADSTRING);

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MWinMgr::MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NBKSHELL));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_NBKSHELL);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	if (!RegisterClassEx(&wcex))
		return false;

	mHwnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 700, 700, NULL, NULL, hInstance, this);

	if (!mHwnd)
		return false;

	mShell = new NbkShell();
	if (!mShell->Create(mHwnd))
		return false;

	mAddressEdit = new MEdit();
	mAddressEdit->Create(mHwnd, IDC_ADDREDIT);
	mAddressEdit->SetListener(mEditListener);

	mGoPrev = new MButton();
	mGoPrev->Create(mHwnd, IDC_GO_PREV);

	mGoNext = new MButton();
	mGoNext->Create(mHwnd, IDC_GO_NEXT);

	SendMessage(mAddressEdit->GetHwnd(), WM_SETFONT, (WPARAM)winMgr->GetMainFont(), FALSE);

	SetWindowPos(mAddressEdit->GetHwnd(), NULL, 2, 2, 600, 30, SWP_NOZORDER);
	SetWindowPos(mGoPrev->GetHwnd(), NULL, 604, 2, 30, 30, SWP_NOZORDER);
	SetWindowPos(mGoNext->GetHwnd(), NULL, 636, 2, 30, 30, SWP_NOZORDER);

	SetWindowPos(mShell->GetHwnd(), NULL, 2, 32, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	return true;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void MainWin::OnCreate()
{
	// 打开控制台，显示标准输出
	if (AllocConsole())
		freopen("CONOUT$", "w", stdout);
}

void MainWin::OnDestroy()
{
	PostQuitMessage(0);
}

bool MainWin::OnCommand(int id, int evt)
{
	switch (id) {
	case IDM_ABOUT:
		DialogBox(MWinMgr::GetInstance()->GetAppInstance(), MAKEINTRESOURCE(IDD_ABOUTBOX), mHwnd, About);
		return true;

	case IDM_EXIT:
		DestroyWindow(mHwnd);
		return true;

	case IDM_PIC:
		mShowPic = !mShowPic;
		mShell->ShowPicture(mShowPic);
		return true;

	case IDC_ADDREDIT:
		return false;

	case IDC_GO_PREV:
		if (MButton::IsClicked(evt)) {
			GoPrev();
			return true;
		}
		break;

	case IDC_GO_NEXT:
		if (MButton::IsClicked(evt)) {
			GoNext();
			return true;
		}
		break;
	}

	return false;
}

void MainWin::LoadUrlFromAddress()
{
	LPTSTR text = mAddressEdit->GetText();
	if (text) {
		int len = wcslen(text);
		if (wcsstr(text, _T("://")) == NULL) {
			len += 7;
			TCHAR* t = new TCHAR[len + 1];
			wcscpy(t, _T("http://"));
			wcscat(t, text);
			delete text;
			text = t;
		}
		char* url = uni_utf16_to_utf8_str((wchr*)text, len, NULL);
		mShell->LoadUrl(url, true);
		delete text;
	}
}

void MainWin::GoPrev()
{
	mShell->LoadPrevPage();
}

void MainWin::GoNext()
{
	mShell->LoadNextPage();
}
