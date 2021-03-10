#pragma once

#ifdef __WIN32__
	#include "win32/jni.h"
#endif
#ifdef __ANDROID__
	#include <jni.h>
#endif
#include "../../stdc/render/imagePlayer.h"

class ImageMgr {

public:
	JNIEnv*		JNI_env;
	jweak		OBJ_ImageMgr;
	jmethodID	MID_createImage;
	jmethodID	MID_deleteImage;
	jmethodID	MID_onData;
	jmethodID	MID_onDataEnd;
	jmethodID	MID_decodeImage;
	jmethodID	MID_drawImage;
	jmethodID	MID_drawBgImage;

private:

	static const int MAX_IMAGES = 200;

	NImage*	mImages[MAX_IMAGES];

	NImagePlayer*	mPlayer;

public:
	ImageMgr();
	~ImageMgr();

	void RegisterMethods(JNIEnv* env, jobject obj);

	void Create(NImage* image, NImagePlayer* player);
	void Delete(NImage* image);
	void Decode(NImage* image);

	void OnData(NImage* image, void* data, int length);
	void OnDataEnd(NImage* image, nbool ok);
	void OnDecodeOver(int id, jboolean ok, int w, int h);

	void Draw(NImage* image, const NRect* rect);
	void DrawBg(NImage* image, const NRect* rect, int rx, int ry);

	void StopAll();

private:

	bool IsAvailable(int id);
	bool IsExist(int id);
};
