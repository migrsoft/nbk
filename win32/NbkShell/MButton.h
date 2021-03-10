#pragma once

#include "MWin.h"

class MButton : public MWin {

public:
	class Listener {
	public:
		virtual void OnClicked(int id) = 0;
	};

public:
	MButton();
	virtual ~MButton();

	virtual bool Create(HWND parent, int id);

	static bool IsClicked(int evt)  { return (evt == BN_CLICKED); }
};
