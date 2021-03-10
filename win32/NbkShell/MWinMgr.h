#pragma once

class MWin;

class MWinMgr {

private:
	static MWinMgr*	mInstance;

	HINSTANCE	mAppInstance;
	HACCEL		mAccelTable;

	HFONT		mMainFont;

public:
	static void Init(HINSTANCE hInstance);
	static void Quit();
	static MWinMgr* GetInstance(); 

	// 惟一注册窗口过程
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	MWinMgr(HINSTANCE hInstance);
	~MWinMgr();

	HINSTANCE GetAppInstance();

	void LoadAccelerators(int resId);

	HFONT GetMainFont();

	int MainLoop();
};
