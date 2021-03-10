#pragma once

#include "MWin.h"

class MEdit : public MWin {

public:
	class Listener {
	public:
		virtual void OnEnter(int id, bool down) = 0;
	};

private:
	LONG_PTR	mDefProc;

	Listener*	mListener;

public:
	MEdit();
	virtual ~MEdit();

	void SetListener(Listener* l) { mListener = l; }

	virtual bool Create(HWND parent, int id);

	virtual LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

	virtual bool OnKeyDown(int key, LPARAM lParam);
	virtual bool OnKeyUp(int key, LPARAM lParam);

	LPTSTR GetText();
};
