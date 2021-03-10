#include "stdafx.h"
#include "MWin.h"

int MWin::mUniqueId = 0;

MWin::MWin()
{
	mWinId = ++mUniqueId;
	mHwnd = NULL;
}

MWin::~MWin()
{
}

LRESULT MWin::WndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(mHwnd, message, wParam, lParam);
}

void MWin::Show(int nCmdShow)
{
	ShowWindow(mHwnd, nCmdShow);
}

void MWin::Update()
{
	UpdateWindow(mHwnd);
}
