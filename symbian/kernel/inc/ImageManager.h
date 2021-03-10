/*
 ============================================================================
 Name		: ImageManager.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CImageManager declaration
 ============================================================================
 */

#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

// INCLUDES
#include "../../../stdc/inc/config.h"
#include "../../../stdc/inc/nbk_gdi.h"
#include <e32std.h>
#include <e32base.h>
#include "imagePlayer.h"

// CLASS DECLARATION

class CNbkGdi;
class CImageImpl;
class CProbe;
class CNBrKernel;

/**
 *  CImageManager
 * 
 */
class CImageManager : public CBase
{
public:
    ~CImageManager();
    static CImageManager* NewL();
    static CImageManager* NewLC();

    void CreateImage(NImagePlayer* player, NImage* image);
    void DeleteImage(NImagePlayer* player, NImage* image);
    void DecodeImage(NImagePlayer* player, NImage* image);
    void BuildAnimationFrame(NImagePlayer* player, NImage* image);
    
    void DrawImage(NImagePlayer* player, NImage* image, const NRect* rect);
    void DrawImage(NImagePlayer* player, NImage* image, const NRect* rect, int rx, int ry);

    void OnImageData(NImage* image, void* data, int length);
    void OnImageDataEnd(NImage* image, TBool succeed);
    void OnImageDecodeOver(CImageImpl* aImage, TBool aSucceed);
    
    void Stop();
    
    float GetCurrentZoomFactor();
    TDisplayMode GetDisplayMode();
    
public:
    CNbkGdi* iGdi;
    CProbe* iProbe;
    CNBrKernel* iNbk;

private:
    CImageManager();
    void ConstructL();
    
    CImageImpl* GetImageImpl(NImage* image);
    
private:
    NImagePlayer*   iPlayer;
    RPointerArray<CImageImpl>   iList;
};

#endif // IMAGEMANAGER_H
