#pragma once

#include "MWin.h"
#include "MEdit.h"
#include "MButton.h"

class NbkShell;

class MainWin : public MWin {

private:
	class EditListener : public MEdit::Listener {
	private:
		MainWin*	mClient;
	public:
		EditListener(MainWin* w) { mClient = w; }
		virtual void OnEnter(int id, bool down);
	};

private:
	static const int MAX_LOADSTRING = 100;

	TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
	TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

	NbkShell*	mShell;
	MEdit*		mAddressEdit;
	MButton*	mGoPrev;
	MButton*	mGoNext;

	EditListener*	mEditListener;

	bool		mShowPic;

public:
	MainWin();
	virtual ~MainWin();

	virtual bool Create(HWND parent);

	virtual void OnCreate();
	virtual void OnDestroy();
	virtual bool OnCommand(int id, int evt);

	void LoadUrlFromAddress();
	void GoPrev();
	void GoNext();
};
