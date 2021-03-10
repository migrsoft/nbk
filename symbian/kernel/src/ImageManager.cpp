/*
 ============================================================================
 Name		: ImageManager.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CImageManager implementation
 ============================================================================
 */

#include "../../../stdc/render/imagePlayer.h"
#include "../../../stdc/tools/str.h"
#include <coemain.h>
#include "ImageManager.h"
#include "ImageImpl.h"
#include "NBKPlatformData.h"
#include "NbkGdi.h"
#include "Probe.h"
#include "NBrKernel.h"
#include "ResourceManager.h"

void NBK_imageCreate(void* pfd, NImagePlayer* player, NImage* image)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->CreateImage(player, image);
}

void NBK_imageDelete(void* pfd, NImagePlayer* player, NImage* image)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->DeleteImage(player, image);
}

void NBK_imageDecode(void* pfd, NImagePlayer* player, NImage* image)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->DecodeImage(player, image);
}

void NBK_imageDraw(void* pfd, NImagePlayer* player, NImage* image, const NRect* rect)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->iGdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    self->DrawImage(player, image, rect);
}

void NBK_imageDrawBg(void* pfd, NImagePlayer* player, NImage* image, const NRect* rect, int rx, int ry)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->iGdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    self->DrawImage(player, image, rect, rx, ry);
}

void NBK_onImageData(void* pfd, NImage* image, void* data, int length)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->OnImageData(image, data, length);
}

void NBK_onImageDataEnd(void* pfd, NImage* image, nbool succeed)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->OnImageDataEnd(image, succeed);
}

void NBK_imageStopAll(void* pfd)
{
    CImageManager* self = (CImageManager*)((NBK_PlatformData*)pfd)->imageManager;
    self->Stop();
}

////////////////////////////////////////////////////////////

CImageManager::CImageManager()
{
}

CImageManager::~CImageManager()
{
    iList.Close();
}

CImageManager* CImageManager::NewLC()
{
    CImageManager* self = new (ELeave) CImageManager();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CImageManager* CImageManager::NewL()
{
    CImageManager* self = CImageManager::NewLC();
    CleanupStack::Pop(); // self;
    return self;
}

void CImageManager::ConstructL()
{
    RFs& fs = CCoeEnv::Static()->FsSession();
    fs.MkDirAll(KImgPath);
}

void CImageManager::CreateImage(NImagePlayer* player, NImage* image)
{
    CImageImpl* im = CImageImpl::NewL(image);
    im->SetManager(this);
    iList.Append(im);
    iPlayer = player;
}

void CImageManager::DeleteImage(NImagePlayer* player, NImage* image)
{
    for (int i=0; i < iList.Count(); i++) {
        if (iList[i]->Compare(image)) {
            delete iList[i];
            iList.Remove(i);
            break;
        }
    }
    iPlayer = player;
}

void CImageManager::DecodeImage(NImagePlayer* player, NImage* image)
{
    iPlayer = player;
    CImageImpl* im = GetImageImpl(image);
    if (im)
        im->Decode();
}

void CImageManager::DrawImage(NImagePlayer* player, NImage* image, const NRect* rect)
{
    iPlayer = player;
    CImageImpl* im = GetImageImpl(image);
    if (im)
        im->Draw(iGdi, rect);
}

void CImageManager::DrawImage(NImagePlayer* player, NImage* image, const NRect* rect, int rx, int ry)
{
    iPlayer = player;
    CImageImpl* im = GetImageImpl(image);
    if (im)
        im->Draw(iGdi, rect, rx, ry);
}

void CImageManager::OnImageDecodeOver(CImageImpl* aImage, TBool aSucceed)
{
    imagePlayer_onDecodeOver(iPlayer, aImage->GetId(), (nbool)aSucceed);
}

void CImageManager::OnImageData(NImage* image, void* data, int length)
{
    CImageImpl* im = GetImageImpl(image);
    if (im)
        im->OnData(data, length);
}

void CImageManager::OnImageDataEnd(NImage* image, TBool succeed)
{
    CImageImpl* im = GetImageImpl(image);
    if (im) {
        if (succeed)
            im->OnComplete();
        else
            im->OnError();
    }
}

CImageImpl* CImageManager::GetImageImpl(NImage* image)
{
    for (int i=0; i < iList.Count(); i++) {
        if (iList[i]->Compare(image))
            return iList[i];
    }
    
    {
        char* crash = NULL;
        if (iProbe) {
            iProbe->OutputTime();
            iProbe->OutputChar("ImageMgr [ image not found ]", -1);
            iProbe->OutputReturn();
            iProbe->Flush();
        }
        *crash = 88;
    }
    
    return NULL;
}

void CImageManager::Stop()
{
//    if (iProbe) {
//        iProbe->OutputChar("ImageMgr: stop all", -1);
//        iProbe->OutputInt(iList.Count());
//        iProbe->OutputReturn();
//    }
    
    for (int i=0; i < iList.Count(); i++)
        iList[i]->Cancel();
}

float CImageManager::GetCurrentZoomFactor()
{
    return iNbk->GetCurZoom();
}

TDisplayMode CImageManager::GetDisplayMode()
{
    return iNbk->GetDisplayMode();
}
