#include "stdafx.h"
#include "MEdit.h"
#include "MWinMgr.h"

MEdit::MEdit()
{
	mListener = NULL;
}

MEdit::~MEdit()
{
}

bool MEdit::Create(HWND parent, int id)
{
	mControlId = id;

	mHwnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
		0, 0, 0, 0, parent, (HMENU)id,
		MWinMgr::GetInstance()->GetAppInstance(), this);

	mDefProc = GetWindowLongPtr(mHwnd, GWLP_WNDPROC);
	SetWindowLongPtr(mHwnd, GWLP_WNDPROC, (LONG_PTR)MWinMgr::MainWndProc);

	SetWindowLongPtr(mHwnd, GWLP_USERDATA, (LONG_PTR)this);

	return true;
}

LRESULT MEdit::WndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return CallWindowProc((WNDPROC)mDefProc, mHwnd, message, wParam, lParam);
}

bool MEdit::OnKeyDown(int key, LPARAM lParam)
{
	switch (key) {
	case VK_RETURN:
		if (mListener)
			mListener->OnEnter(mControlId, true);
		return true;

	default:
		return false;
	}
}

bool MEdit::OnKeyUp(int key, LPARAM lParam)
{
	switch (key) {
	case VK_RETURN:
		if (mListener)
			mListener->OnEnter(mControlId, false);
		return true;

	default:
		return false;
	}
}

LPTSTR MEdit::GetText()
{
	int len = Edit_GetTextLength(mHwnd);
	if (len == 0)
		return NULL;

	LPTSTR p = new TCHAR[len + 1];
	Edit_GetText(mHwnd, p, len + 1);

	return p;
}
