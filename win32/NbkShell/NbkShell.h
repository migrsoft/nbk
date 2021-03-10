#pragma once

#include "MWin.h"

using namespace Gdiplus;

class NbkCore;
class NbkEvent;

class NbkShell : public MWin {

private:
	TCHAR	mClassName[12];

	int			mScreenWidth;
	int			mScreenHeight;

	int			mPageWidth;
	int			mPageHeight;

	Bitmap*		mCanvas;
	Bitmap*		mScreen;
	Graphics*	mGdi;

	NbkCore*	mCore;
	HANDLE		mCoreThread;

public:
	NbkShell();
	virtual ~NbkShell();

	virtual bool Create(HWND parent);

	virtual void OnCreate();
	virtual void OnDestroy();
	virtual bool OnPaint();
	virtual void OnTimer(WPARAM wParam);
	virtual void OnUser(WPARAM wParam, LPARAM lParam);
	virtual void OnVertScroll(int value, int pos);
	virtual void OnHorzScroll(int value, int pos);
	virtual bool OnLeftButton(bool down, int x, int y);

	int GetScreenWidth() { return mScreenWidth; }
	int GetScreenHeight() { return mScreenHeight; }
	Graphics* GetGdi() { return mGdi; }

	void PostEvent(NbkEvent* evt);

	void StartTimer(int id, int interval);
	void StopTimer(int id);

	void LoadUrl(const char* url, bool del = false);
	void LoadPrevPage();
	void LoadNextPage();

	void ShowPicture(bool show);

private:
	void OnPageSizeChanged(int w, int h);
	void CalcScrollInfo(LPSCROLLINFO si, int view, int extent);
	void UpdateScrollbar(BOOL redraw);
	void ScrollPage();
	void OnPageUpdate();
	void OnScrollByPage(int sbar, bool down);
	void OnNewDocumentBegin();
};
