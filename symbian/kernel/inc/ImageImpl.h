/*
 ============================================================================
 Name		: ImageImpl.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CImageImpl declaration
 ============================================================================
 */

#ifndef IMAGEIMPL_H
#define IMAGEIMPL_H

#include <e32base.h>	// For CActive, link against: euser.lib
#include <e32std.h>		// For RTimer, link against: euser.lib
#include <f32file.h>
#include <fbs.h>
#include <ImageConversion.h>
#include "imagePlayer.h"

_LIT(KImgPath, "c:\\data\\nbk\\");

class CNbkGdi;
class CImageManager;


class CImageImplFrame : public CBase
{
public:
    CImageImplFrame();
    ~CImageImplFrame();
    
    CFbsBitmap* iFrame;
    CFbsBitmap* iMask;
};

// -----------------------------------------------------------------------------
// 图片解码、绘制
// -----------------------------------------------------------------------------

class CImageImpl : public CActive
{    
public:
    ~CImageImpl();

    static CImageImpl* NewL(NImage* image);
    static CImageImpl* NewLC(NImage* image);

public:
    TInt16 GetId() { return d->id; }
    void SetManager(CImageManager* aMgr);
    
    void OnData(void* data, int length);
    void OnComplete();
    void OnError();
    
    TBool Compare(const NImage* image);
    void Decode();
    void Cancel();
    
    // 绘制普通图片
    void Draw(CNbkGdi* gdi, const NRect* rect);
    // 绘制背景贴图
    void Draw(CNbkGdi* gdi, const NRect* rect, int rx, int ry);
    
private:
    
    // C++ constructor
    CImageImpl(NImage* image);

    // Second-phase constructor
    void ConstructL();

    void BuildAnimationFrameL();
    void ClearFrames();
    void ClearDecoder();
    void ClearDataStream();
    //纯色图片 return ETrue，其它返回EFalse
    TBool ParseOnePixelImag(CNbkGdi* aGdi, const TRect& aRect);
//    CFbsBitmap* StretchIt(const CFbsBitmap* aSrc, const TSize& aSize, const TRect& aSrcR);
//    CFbsBitmap* StretchBitmap(const CFbsBitmap* aSrc, const TSize& aSize, const TRect& aSrcR);
//    CFbsBitmap* StretchBitmapNinePalace(const CFbsBitmap* aSrc, const TSize& aSize, const TRect& aSrcR);
//    void StretchFrames();
    
private:
    // From CActive
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);
    void SelfComplete(TInt aError);

    HBufC* GetFileName();
    void CreateTempBitmap(const CImageImplFrame* aFrame,
                          CFbsBitmap*& aBmp, CFbsBitmap*& aMask, TRect aScaleTo);
    void ClearTempBitmaps();

private:
    NImage* d;
    
    CImageManager* iMgr;
    CImageDecoder* iDecoder;
    
    TInt16 iCurr;
    TBool iSkip;
    TBool iDestroy;
    
//    int iStretch; // 图片拉伸方式

    TFrameInfo iFrameInfo;
    RPointerArray<CImageImplFrame> iFrames; // 存储解码帧
    TUint32 iColor; //for 单色图片 
    // 缓存缩放图片
    CFbsBitmap* iScaledBitmap;
    CFbsBitmap* iScaledMask;
    float       iScaleZoomFactor;
    
    RFile iFile;
    TBool iFileOpen;
    HBufC* iFileName;

    TUint8* iSrcData;
    TInt iDataLen;
    TPtrC8 iDataPtr;
    TBool iReceOver; // 图片数据是否接收完成
    int t_time;
};

class BitmapUtil
{
public:
    static TInt CopyBitmapData(const CFbsBitmap& aSource, CFbsBitmap& aDestination,
        const TSize& aSize, const TDisplayMode& aDisplayMode);
};

#endif // IMAGEIMPL_H
