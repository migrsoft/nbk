#pragma once

#include "../../stdc/inc/config.h"
#include "../../stdc/inc/nbk_gdi.h"

using namespace Gdiplus;

class NbkShell;

class FontW32 {

private:
	int		mPixel;
	nbool	mBold;
	nbool	mItalic;

	REAL	mAscent;
	int		mHeight;
	int		mHzWidth;

	Font*	mFont;

public:
	FontW32(Graphics* g, int pixel, nbool bold, nbool italic);
	~FontW32();

	bool Equals(int pixel, nbool bold, nbool italic);
	Font* GetFont() { return mFont; }
	int GetHeight() { return mHeight; }
	int GetTextWidth(Graphics* g, const wchr* text, int length);
	REAL GetAscent() { return mAscent; }
};

class GraphicsW32 {

private:
	static const int MAX_FONTS = 32;

	NbkShell*	mShell;

	NColor	mPenColor;
	NColor	mBrushColor;

	Pen*		mPen;
	SolidBrush*	mBrush;
	SolidBrush*	mTextBrush;

	FontW32*	mFonts[MAX_FONTS];
	int			mCurFont;

public:
	GraphicsW32();
	~GraphicsW32();

	void SetShell(NbkShell* shell) { mShell = shell; }

	void Reset();

	NFontId GetFont(int16 pixel, nbool bold, nbool italic);
	coord GetFontHeight(NFontId id);
	coord GetCharWidth(NFontId id, const wchr ch);
	coord GetTextWidth(NFontId id, const wchr* text, int length);
	void UseFont(NFontId id);

	void SetPenColor(const NColor* color);
	void SetBrushColor(const NColor* color);

	void SetClip(const NRect* clip);
	void CancelClip();

	void DrawText(const wchr* text, int length, const NPoint* pos, int decor);
	void DrawRect(const NRect* rect);
	void FillRect(const NRect* rect);
	void DrawEllipse(const NRect* rect);
	void DrawImage(Image* image, int x, int y, int w, int h);

private:
	bool IsExist(int id);
};
