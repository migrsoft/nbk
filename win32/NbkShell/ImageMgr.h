#pragma once

#include "../../stdc/render/imagePlayer.h"
#include "../../stdc/tools/strBuf.h"

using namespace Gdiplus;

class NbkCore;
class NbkEvent;
class GraphicsW32;

class ImageW32 {

private:
	int			mId;
	int			mPageId;
	NStrBuf*	mData;
	Image*		mImage;
	bool		mInQueue; // 当对象在消息队列中，清除图像任务时不删除对象，待消息队列通知后，再删除

public:
	NImage*		mNIMAGE; // 可能被外部删除

public:
	ImageW32(NImage* im);
	~ImageW32();

	void OnData(char* data, int length);
	void OnDataEnd(nbool ok);

	bool Decode();

	void SetInQueue(bool in) { mInQueue = in; }
	bool IsInQueue() { return mInQueue; }

	int GetId() { return mId; }
	int GetPageId() { return mPageId; }

	void Draw(GraphicsW32* g, int x, int y, int w, int h);
	void DrawRepeat(GraphicsW32* g, int x, int y, int w, int h, int rx, int ry);
};

class ImageMgr {

private:
	static const int MAX_IMAGES = 200;

	NbkCore*	mCore;

	ImageW32*	mImages[MAX_IMAGES];

	NImagePlayer*	mPlayer;

public:
	ImageMgr(NbkCore* core);
	~ImageMgr();


	void Create(NImage* image, NImagePlayer* player);
	void Delete(NImage* image);
	void Decode(NImage* image);

	void OnData(NImage* image, void* data, int length);
	void OnDataEnd(NImage* image, nbool ok);

	void Draw(NImage* image, const NRect* rect);
	void DrawBg(NImage* image, const NRect* rect, int rx, int ry);

	void StopAll();

	void HandleEvent(NbkEvent* evt);

private:

	bool IsAvailable(int id);
	bool IsExist(int id);
};
