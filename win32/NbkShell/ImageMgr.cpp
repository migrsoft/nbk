#include "stdafx.h"
#include "ImageMgr.h"
#include "NbkCore.h"
#include "NbkEvent.h"
#include "Graphic.h"

////////////////////////////////////////////////////////////////////////////////
//
// NBK 图像资源请求
//

void NBK_imageCreate(void* pfd, NImagePlayer* player, NImage* image)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->Create(image, player);
}

void NBK_imageDelete(void* pfd, NImagePlayer* player, NImage* image)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->Delete(image);
}

void NBK_imageDecode(void* pfd, NImagePlayer* player, NImage* image)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->Decode(image);
}

void NBK_imageDraw(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->Draw(image, dest);
}

void NBK_imageDrawBg(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest, int rx, int ry)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->DrawBg(image, dest, rx, ry);
}

void NBK_onImageData(void* pfd, NImage* image, void* data, int length)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->OnData(image, data, length);
}

void NBK_onImageDataEnd(void* pfd, NImage* image, nbool succeed)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->OnDataEnd(image, succeed);
}

void NBK_imageStopAll(void* pfd)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetImageMgr()->StopAll();
}

//nbool NBK_isImageCached(void* pfd, const char* url, nbool history)
//{
//	NbkCore* core = (NbkCore*)pfd;
//	return N_FALSE;
//}

////////////////////////////////////////////////////////////////////////////////
//
// ImageW32 图片实现
//

ImageW32::ImageW32(NImage* im)
{
	mId = im->id;
	mPageId = im->pageId;
	mNIMAGE = im;
	mData = NULL;
	mImage = NULL;
	mInQueue = false;
}

ImageW32::~ImageW32()
{
	if (mData)
		strBuf_delete(&mData);
	if (mImage)
		delete mImage;
}

void ImageW32::OnData(char* data, int length)
{
	if (mData == NULL)
		mData = strBuf_create();

	strBuf_appendStr(mData, (char*)data, length);
}

void ImageW32::OnDataEnd(nbool ok)
{
	if (!ok && mData)
		strBuf_delete(&mData);
}

bool ImageW32::Decode()
{
	if (mData == NULL)
		return false;

	char* str;
	int size = 0;
	int len;
	nbool begin = N_TRUE;

	while (strBuf_getStr(mData, &str, &len, begin)) {
		begin = N_FALSE;
		size += len;
	}

	HGLOBAL hMem = GlobalAlloc(GHND, size);
	char* p = (char*)GlobalLock(hMem);

	begin = N_TRUE;
	size = 0;
	while (strBuf_getStr(mData, &str, &len, begin)) {
		begin = N_FALSE;
		NBK_memcpy(p + size, str, len);
		size += len;
	}
	strBuf_delete(&mData);

	GlobalUnlock(hMem);

	IStream* stream = NULL;
	CreateStreamOnHGlobal(hMem, TRUE, &stream);

	mImage = Image::FromStream(stream);
	if (mImage->GetLastStatus() == Ok) {
		mNIMAGE->size.w = mImage->GetWidth();
		mNIMAGE->size.h = mImage->GetHeight();
		mNIMAGE->total = mImage->GetFrameDimensionsCount();
	}
	else {
		delete mImage;
		mImage = NULL;
	}

	stream->Release();

	return (mImage != NULL);
}

void ImageW32::Draw(GraphicsW32* g, int x, int y, int w, int h)
{
	if (mImage)
		g->DrawImage(mImage, x, y, w, h);
}

void ImageW32::DrawRepeat(GraphicsW32* g, int x, int y, int w, int h, int rx, int ry)
{
	if (mImage) {
		for (int i = 0; i <= rx; i++) {
			for (int j = 0; j <= ry; j++) {
				g->DrawImage(mImage, x + i * w, y + j * h, w, h);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// ImageMgr 图片管理器
//

ImageMgr::ImageMgr(NbkCore* core)
{
	mCore = core;
	NBK_memset(&mImages, 0, sizeof(ImageW32*) * MAX_IMAGES);
}

ImageMgr::~ImageMgr()
{
}

void ImageMgr::Create(NImage* image, NImagePlayer* player)
{
	mPlayer = player;
	if (IsAvailable(image->id)) {
		mImages[image->id] = new ImageW32(image);
	}
}

void ImageMgr::Delete(NImage* image)
{
	if (IsExist(image->id)) {
		if (!mImages[image->id]->IsInQueue())
			delete mImages[image->id];
		mImages[image->id] = NULL;
	}
}

void ImageMgr::Decode(NImage* image)
{
	if (IsExist(image->id)) {
		NbkEvent* e = new NbkEvent(NbkEvent::EVENT_DECODE_IMG);
		e->SetUser(mImages[image->id]);
		mImages[image->id]->SetInQueue(true);
		mCore->PostEvent(e);
	}
}

void ImageMgr::OnData(NImage* image, void* data, int length)
{
	if (IsExist(image->id)) {
		mImages[image->id]->OnData((char*)data, length);
	}
}

void ImageMgr::OnDataEnd(NImage* image, nbool ok)
{
	if (IsExist(image->id)) {
		mImages[image->id]->OnDataEnd(ok);
	}
}

void ImageMgr::Draw(NImage* image, const NRect* r)
{
	if (IsExist(image->id)) {
		GraphicsW32* g = mCore->GetGraphic();
		mImages[image->id]->Draw(g, r->l, r->t, rect_getWidth(r), rect_getHeight(r));
	}
}

void ImageMgr::DrawBg(NImage* image, const NRect* r, int rx, int ry)
{
	if (IsExist(image->id)) {
		GraphicsW32* g = mCore->GetGraphic();
		mImages[image->id]->DrawRepeat(g, r->l, r->t, rect_getWidth(r), rect_getHeight(r), rx, ry);
	}
}

bool ImageMgr::IsAvailable(int id)
{
	return (id >= 0 && id < MAX_IMAGES);
}

bool ImageMgr::IsExist(int id)
{
	return (IsAvailable(id) && mImages[id]);
}

void ImageMgr::StopAll()
{
	for (int i=0; i < MAX_IMAGES; i++) {
		if (mImages[i]) {
			if (!mImages[i]->IsInQueue())
				delete mImages[i];
			mImages[i] = NULL;
		}
	}
}

void ImageMgr::HandleEvent(NbkEvent* evt)
{
	switch (evt->GetId()) {
	case NbkEvent::EVENT_DECODE_IMG:
	{
		ImageW32* im = (ImageW32*)evt->GetUser();
		im->SetInQueue(false);
		NPage* page = mCore->GetPage();
		if (IsExist(im->GetId()) && page_getId(page) == im->GetPageId()) {
			nbool ok = (nbool)im->Decode();
			imagePlayer_onDecodeOver(mPlayer, im->mNIMAGE->id, ok);
		}
		else {
			delete im;
		}
		break;
	}
	}
}
