#include "stdafx.h"
#include "MButton.h"
#include "MWinMgr.h"

MButton::MButton()
{
}

MButton::~MButton()
{
}

bool MButton::Create(HWND parent, int id)
{
	mControlId = id;

	mHwnd = CreateWindowEx(WS_EX_WINDOWEDGE, TEXT("button"), NULL,
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		0, 0, 0, 0, parent, (HMENU)id,
		MWinMgr::GetInstance()->GetAppInstance(), this);

	SetWindowLongPtr(mHwnd, GWLP_USERDATA, (LONG_PTR)this);

	return true;
}
