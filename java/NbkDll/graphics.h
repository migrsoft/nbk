#pragma once

#ifdef __WIN32__
	#include "win32/jni.h"
#endif
#ifdef __ANDROID__
	#include <jni.h>
#endif
#include "../../stdc/inc/config.h"
#include "../../stdc/inc/nbk_gdi.h"

class Graphic {

private:
	JNIEnv*		JNI_env;
	jweak		OBJ_graphic;

	jmethodID	MID_reset;

	jmethodID	MID_getFont;
	jmethodID	MID_getFontHeight;
	jmethodID	MID_getTextWidth;
	jmethodID	MID_useFont;

	jmethodID	MID_setColor;
	jmethodID	MID_setBgColor;

	jmethodID	MID_setClip;
	jmethodID	MID_cancelClip;

	jmethodID	MID_drawText;
	jmethodID	MID_drawRect;
	jmethodID	MID_fillRect;
	jmethodID	MID_drawOval;

private:
	NColor	mColor;
	NColor	mBgColor;

public:
	Graphic();
	~Graphic();

	void RegisterJavaMethods(JNIEnv* env, jobject obj);

	void Reset();

	NFontId GetFont(int16 pixel, nbool bold, nbool italic);
	coord GetFontHeight(NFontId id);
	coord GetCharWidth(NFontId id, const wchr ch);
	coord GetTextWidth(NFontId id, const wchr* text, int length);
	void UseFont(NFontId id);

	void SetColor(const NColor* color);
	void SetBgColor(const NColor* color);

	void SetClip(const NRect* clip);
	void CancelClip();

	void DrawText(const wchr* text, int length, const NPoint* pos, int decor);
	void DrawRect(const NRect* rect);
	void ClearRect(const NRect* rect);
	void DrawOval(const NRect* rect);
};
