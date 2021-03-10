#pragma once

class MWin {

private:
	static int mUniqueId;

protected:
	int		mWinId;
	HWND	mHwnd;
	int		mControlId;

public:
	MWin();
	virtual ~MWin();

	HWND GetHwnd() const { return mHwnd; }
	void SetHwnd(HWND hwnd) { mHwnd = hwnd; }

	virtual bool Create(HWND parent) { return false; }
	virtual void Show(int nCmdShow);
	virtual void Update();

	virtual LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

	virtual void OnCreate() {}
	virtual void OnDestroy() {}
	virtual bool OnPaint() { return false; }
	virtual bool OnCommand(int id, int evt) { return false; }
	virtual void OnTimer(WPARAM wParam) {}
	virtual void OnUser(WPARAM wParam, LPARAM lParam) {}

	virtual bool OnKeyDown(int key, LPARAM lParam) { return false; }
	virtual bool OnKeyUp(int key, LPARAM lParam) { return false; }

	virtual bool OnLeftButton(bool down, int x, int y) { return false; }

	virtual void OnVertScroll(int value, int pos) {};
	virtual void OnHorzScroll(int value, int pos) {};
};
