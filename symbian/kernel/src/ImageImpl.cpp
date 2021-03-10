/*
 ============================================================================
 Name		: ImageImpl.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CImageImpl implementation
 ============================================================================
 */

#include "../../../stdc/inc/nbk_cbDef.h"
#include "../../../stdc/inc/nbk_limit.h"
#include "../../../stdc/css/css_val.h"
#include "../../../stdc/css/css_helper.h"
#include <coemain.h>
#include <bautils.h>
#include <w32std.h>
#include <math.h>
#include "ImageImpl.h"
#include "ImageManager.h"
#include "NbkGdi.h"
#include "Probe.h"
#include "NBrKernel.h"
#include "ResourceManager.h"
#include "DecodeThread.h"

#define DEBUG_DECODE        0
#define CACHE_IN_MEM        1
#define MAX_REPEAT_X        500
#define MAX_REPEAT_Y        20
#define DECODE_THREAD       1
//#define TIME_LOG

#define TEST_IMAGE_CONSUME  0

// 图片屏蔽策略
//const float KPicDenyPolicy = 0.6; // 当缩放因子小于该值是，启动图片屏蔽策略
//const int KMinPicW = 50;
//const int KMaxPicW = 300;
const TInt KMaxDataSize = 102400;//100KB

_LIT(KNameFmt,  "%Scp%05d\\img%05d.png");


CImageImplFrame::CImageImplFrame()
    : iFrame(NULL)
    , iMask(NULL)
{
}

CImageImplFrame::~CImageImplFrame()
{
    if (iFrame) {
        delete iFrame;
        iFrame = NULL;
    }
    if (iMask) {
        delete iMask;
        iMask = NULL;
    }
}

// -----------------------------------------------------------------------------
// 图片解码、绘制
// -----------------------------------------------------------------------------

CImageImpl::CImageImpl(NImage* image)
    : CActive(EPriorityUserInput - 1)
    , d(image)
    , iFileOpen(EFalse)
    , iDecoder(NULL)
    , iSkip(EFalse)
    , iDestroy(EFalse)
    , iScaledBitmap(NULL)
    , iScaledMask(NULL)
    , iSrcData(NULL)
    , iDataLen(0)
    , iScaleZoomFactor(1.0)
    , iReceOver(EFalse)
{
}

CImageImpl* CImageImpl::NewLC(NImage* image)
{
    CImageImpl* self = new (ELeave) CImageImpl(image);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CImageImpl* CImageImpl::NewL(NImage* image)
{
    CImageImpl* self = CImageImpl::NewLC(image);
    CleanupStack::Pop(); // self;
    return self;
}

void CImageImpl::ConstructL()
{
    CActiveScheduler::Add(this); // Add to scheduler
}

CImageImpl::~CImageImpl()
{
    Cancel();

    iFile.Close();

    if (iFileName) {
        CCoeEnv::Static()->FsSession().Delete(iFileName->Des());
        delete iFileName;
    }

    ClearDecoder();
    ClearDataStream();
    ClearFrames();
    ClearTempBitmaps();
}

void CImageImpl::RunL()
{
    if (iSkip) {
        ClearDecoder();
        if (!iDestroy)
            iMgr->OnImageDecodeOver(this, EFalse);
        return;
    }
    TInt res = iStatus.Int();
#if DECODE_THREAD
    
    if (KErrNone == res) {
        iCurr = iFrames.Count();
        BuildAnimationFrameL();
    }
    iMgr->OnImageDecodeOver(this, !res);
    ClearDecoder();   
    ClearDataStream();
#else
    switch (res) {
    case KErrNone:
    {
        if (iDestroy) {
            ClearDecoder();
            return;
        }
        
        iCurr++;
        if (iCurr < d->total) {
            if (iFrames[iCurr]->iMask)
                iDecoder->Convert(&iStatus, *iFrames[iCurr]->iFrame, *iFrames[iCurr]->iMask, iCurr);
            else
                iDecoder->Convert(&iStatus, *iFrames[iCurr]->iFrame, iCurr);
            SetActive();
        }
        else {
#if DEBUG_DECODE
            iMgr->iProbe->OutputChar("IMG decode finish time = ", -1);
            //iMgr->iProbe->OutputInt(d->id);
            iMgr->iProbe->OutputInt(NBK_currentMilliSeconds() - t_time);
            iMgr->iProbe->OutputReturn();
#endif
            
            ClearDataStream();
            
            if (d->sti)
                BackgroundStretch();
            else
                BuildAnimationFrameL();
#ifdef TIME_LOG
            iMgr->iProbe->OutputTime();
            iMgr->iProbe->OutputChar("decode over", -1);
            iMgr->iProbe->OutputChar(d->origSrc, -1);
            iMgr->iProbe->OutputReturn();
#endif
            
            if (!iDestroy)
                iMgr->OnImageDecodeOver(this, ETrue);
            iDestroy = ETrue;
            SelfComplete(KErrNone);
        }
        break;
    }
    case KErrAlreadyExists:
        iMgr->OnImageDecodeOver(this, ETrue);
        break;
        
    case KErrCorrupt:
        iMgr->OnImageDecodeOver(this, EFalse);
        break;
        
    default:
    {
        ClearFrames();
        ClearDataStream();
        if (!iDestroy)
            iMgr->OnImageDecodeOver(this, EFalse);
        iDestroy = ETrue;
        SelfComplete(KErrNone);
        break;
    }
    }
#endif
}

void CImageImpl::DoCancel()
{
    if (iDecoder) {
        iDecoder->Cancel();
        if (!iDestroy)
            iMgr->OnImageDecodeOver(this, EFalse);
    }
}

TInt CImageImpl::RunError(TInt aError)
{
    if (!iDestroy)
        iMgr->OnImageDecodeOver(this, EFalse);
    return aError;
}

void CImageImpl::SelfComplete(TInt aError)
{
    if (IsActive())
        Cancel();
    
    SetActive();
    iStatus = KRequestPending;
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, aError);
}

void CImageImpl::OnData(void* data, int length)
{
#if TEST_IMAGE_CONSUME
    int t_time = NBK_currentMilliSeconds();
#endif
    
    if (iReceOver) {
        // 阻止数据重写
        if (iSrcData)
            NBK_free(iSrcData);
        iSrcData = NULL;
        iDataLen = 0;
        iReceOver = EFalse;
    }

#if CACHE_IN_MEM    
    if (iSrcData == NULL) {
        iSrcData = (TUint8*)NBK_malloc(length);
        NBK_memcpy(iSrcData, data, length);
        iDataLen = length;
    }
    else {
        iSrcData = (TUint8*)NBK_realloc(iSrcData, (length + iDataLen));
        NBK_memcpy(iSrcData + iDataLen, data, length);
        iDataLen += length;
    }
#else
    if (!iFileOpen) {
        RFs& rfs = CCoeEnv::Static()->FsSession();
        if (iFileName)
            delete iFileName;
        iFileName = GetFileName();
        rfs.MkDirAll(iFileName->Des());
        if (BaflUtils::FileExists(rfs, iFileName->Des()))
            BaflUtils::DeleteFile(rfs, iFileName->Des());
        if (iFile.Create(rfs, iFileName->Des(), EFileWrite) == KErrNone)
            iFileOpen = ETrue;
    }

    if (iFileOpen) {
        TPtrC8 dataPtr((TUint8*)data, length);
        iFile.Write(dataPtr);
    }
#endif
    
#if TEST_IMAGE_CONSUME 
    iMgr->iProbe->OutputChar("IMG write time = ", -1);
    iMgr->iProbe->OutputInt((NBK_currentMilliSeconds() - t_time));
    iMgr->iProbe->OutputChar("  IMG id = ", -1);
    iMgr->iProbe->OutputInt(d->id);
    iMgr->iProbe->OutputReturn();
#endif
}

void CImageImpl::OnComplete()
{
    iReceOver = ETrue; // 接收数据完成，数据可能为0长度
    
    if (iFileOpen) {
        iFile.Close();
        iFileOpen = EFalse;
    }
}

void CImageImpl::OnError()
{
    iReceOver = ETrue;
    
    ClearDataStream();
    
    if (iFileOpen) {
        iFile.Close();
        iFileOpen = EFalse;
    }
}

HBufC* CImageImpl::GetFileName()
{
    CResourceManager* resMgr = CResourceManager::GetPtr(); 
    int len = resMgr->GetCachePath().Length();
    
    HBufC* name = HBufC::New(len + 64);
    TPtr p = name->Des();
    TPtr path = resMgr->GetCachePath();
    p.Format(KNameFmt, &path, d->pageId, d->id);
    return name;
}

void CImageImpl::Decode()
{
    iDestroy = EFalse;
    
    if (d->total > 0) {
        // 帧数存在，数据流已清除，该图片已解码成功
        SelfComplete((iSrcData) ? KErrCorrupt : KErrAlreadyExists);
        return;
    }

#if CACHE_IN_MEM
    if (!iSrcData || iDataLen < 8) {
        iSkip = ETrue;
        SelfComplete(KErrNone);
        return;
    }
#else
    if (iFileName == NULL) {
        iSkip = ETrue;
        SelfComplete(KErrNone);
        return;
    }
#endif
    
#if TEST_IMAGE_CONSUME
    int t_time = NBK_currentMilliSeconds();
#endif

    CImageDecoder::TOptions opt = (CImageDecoder::TOptions) (CImageDecoder::EOptionAlwaysThread);
        //| CImageDecoder::EPreferFastDecode);
#if CACHE_IN_MEM
    iDataPtr.Set((const TUint8*)iSrcData, iDataLen);
    TRAPD(err, iDecoder = CImageDecoder::DataNewL(CCoeEnv::Static()->FsSession(), iDataPtr));//, opt));
#else
    TRAPD(err, iDecoder = CImageDecoder::FileNewL(CCoeEnv::Static()->FsSession(), iFileName->Des(), opt));
#endif
    
#if TEST_IMAGE_CONSUME 
    iMgr->iProbe->OutputChar("IMG read time = ", -1);
    iMgr->iProbe->OutputInt(NBK_currentMilliSeconds() - t_time);
    iMgr->iProbe->OutputChar("  IMG id = ", -1);
    iMgr->iProbe->OutputInt(d->id);
    iMgr->iProbe->OutputReturn();
#endif
    
    if (err != KErrNone) {
#if DEBUG_DECODE
        iMgr->iProbe->OutputChar("IMG create decoder failed", -1);
        iMgr->iProbe->OutputReturn();
#endif
        iSkip = ETrue;
        SelfComplete(KErrNone);
        return;
    }
        
    d->total = iDecoder->FrameCount();
    iCurr = 0; // 解码起始帧
    
    iFrameInfo = iDecoder->FrameInfo(0);
    
    d->size.w = iFrameInfo.iOverallSizeInPixels.iWidth;
    d->size.h = iFrameInfo.iOverallSizeInPixels.iHeight;
    
#if DEBUG_DECODE
    //iMgr->iProbe->OutputTime();
    t_time = NBK_currentMilliSeconds();
    iMgr->iProbe->OutputChar("IMG decode", -1);
    iMgr->iProbe->OutputInt(d->id);
    iMgr->iProbe->OutputChar(d->src, -1);
    iMgr->iProbe->OutputInt(d->total);
    iMgr->iProbe->OutputInt(d->size.w);
    iMgr->iProbe->OutputInt(d->size.h);
    iMgr->iProbe->OutputReturn();
#endif

#ifdef TIME_LOG
    iMgr->iProbe->OutputTime();
    iMgr->iProbe->OutputChar("decode", -1);
    iMgr->iProbe->OutputChar(d->origSrc, -1);
    iMgr->iProbe->OutputReturn();
#endif

    TDisplayMode dismode = iMgr->GetDisplayMode();
    
    // 创建解码帧
    TDisplayMode maskDisplayMode(ENone);

    for (int i = 0; i < d->total; i++) {

        TFrameInfo frameInfo = iDecoder->FrameInfo(i);

        CImageImplFrame* fr = new (ELeave) CImageImplFrame;
        fr->iFrame = new (ELeave) CFbsBitmap();
        fr->iFrame->Create(frameInfo.iOverallSizeInPixels, dismode);

        if (frameInfo.iFlags & TFrameInfo::ETransparencyPossible) {
            if (frameInfo.iFlags & TFrameInfo::EAlphaChannel && (frameInfo.iFlags
                & TFrameInfo::ECanDither))
                maskDisplayMode = EGray256;
            else
                maskDisplayMode = EGray2;

            fr->iMask = new (ELeave) CFbsBitmap;
            fr->iMask->Create(frameInfo.iOverallSizeInPixels, maskDisplayMode);
        }
        iFrames.Append(fr);
    }
    
#if DECODE_THREAD
    CDecodeThread* decodeThread = CResourceManager::GetPtr()->GetDecodeThread();
    iDataPtr.Set((const TUint8*) iSrcData, iDataLen);
    decodeThread->Decode(&iStatus, iDataPtr, iFrames, dismode);
    SetActive();
#else

    if (iFrames[iCurr]->iMask)
    iDecoder->Convert(&iStatus, *iFrames[iCurr]->iFrame, *iFrames[iCurr]->iMask, iCurr);
    else
    iDecoder->Convert(&iStatus, *iFrames[iCurr]->iFrame, iCurr);

    SetActive();
#endif 
}

void CImageImpl::Cancel()
{
    iReceOver = ETrue;
    iDestroy = ETrue;
    CActive::Cancel();
}

void CImageImpl::SetManager(CImageManager* aMgr)
{
    iMgr = aMgr;
}

// 创建供缩放使用的临时位图
void CImageImpl::CreateTempBitmap(
    const CImageImplFrame* aFrame,
    CFbsBitmap*& aBmp, CFbsBitmap*& aMask,
    TRect aScaleTo)
{
    // 创建缩放图
    CFbsBitmapDevice* dev;
    CFbsBitGc* gc;
    TRect rect(TPoint(0, 0), aScaleTo.Size());
    
    if (   iScaledBitmap
        && d->total == 1
        && fabs(iScaleZoomFactor - iMgr->GetCurrentZoomFactor()) < NFLOAT_PRECISION ) {
        // 使用缓存图
        aBmp = iScaledBitmap;
        aMask = iScaledMask;
        return;
    }
    
    ClearTempBitmaps();
    
    // 创建主图
    aBmp = new CFbsBitmap();
    aBmp->Create(aScaleTo.Size(), aFrame->iFrame->DisplayMode());
    dev = CFbsBitmapDevice::NewL(aBmp);
    dev->CreateContext(gc);
    gc->DrawBitmap(rect, aFrame->iFrame);
    delete gc;
    delete dev;

    // 创建 MASK
    if (aFrame->iMask) {
        aMask = new CFbsBitmap();
        aMask->Create(aScaleTo.Size(), aFrame->iMask->DisplayMode());
        dev = CFbsBitmapDevice::NewL(aMask);
        dev->CreateContext(gc);
        gc->DrawBitmap(rect, aFrame->iMask);
        delete gc;
        delete dev;
    }
    else
        aMask = NULL;
    
    iScaledBitmap = aBmp;
    iScaledMask = aMask;
    iScaleZoomFactor = iMgr->GetCurrentZoomFactor();
}

void CImageImpl::ClearTempBitmaps()
{
    if (iScaledBitmap) {
        delete iScaledBitmap;
        iScaledBitmap = NULL;
    }
    if (iScaledMask) {
        delete iScaledMask;
        iScaledMask = NULL;
    }
}

void CImageImpl::Draw(CNbkGdi* gdi, const NRect* rect)
{
    float zoom = iMgr->GetCurrentZoomFactor();
    TRect dst(rect->l, rect->t, rect->r, rect->b);
    
    if (d->state == NEIS_DECODE_FINISH) {
        if (iCurr < 1)
            return;
        
        TRect src(0, 0, d->size.w, d->size.h);
        TBool deny = EFalse;

        CImageImplFrame* curFrame = iFrames[d->curr];

        if (deny) {
            TRgb color;
            curFrame->iFrame->GetPixel(color, TPoint(0, 0));
            gdi->DrawBitmapPlaceholder(dst, color);
        }
        else {
            CFbsBitmap *bmp, *mask;

            if (fabs(zoom - 1.0) < NFLOAT_PRECISION && dst.Size() == src.Size()) {

                ClearTempBitmaps();
                bmp = curFrame->iFrame;
                mask = curFrame->iMask;
            }
            else {
                CreateTempBitmap(curFrame, bmp, mask, dst);
                src = TRect(TPoint(0, 0), bmp->SizeInPixels());
            }

            if (mask)
                gdi->BitBltMask(dst, bmp, src, mask, EFalse);
            else
                gdi->BitBlt(dst, bmp);
        }
    }
    else {
        // 图片未载入时，绘制白底灰框
        gdi->SetBrushColor(KRgbWhite);
        gdi->ClearRect(dst);
        gdi->SetPenColor(KRgbGray);
        gdi->DrawRect(dst);
    }
}

// 画背景
// DrawBitmap, DrawBitmapMask 在目标矩形原点为较大负值时，存在BUG
// DrawBitmap 在源矩形原点不为0,0时，存在BUG
// DrawBitmapMask 在目标矩形与源矩形不同的情况下，存在BUG
void CImageImpl::Draw(CNbkGdi* gdi, const NRect* rect, int rx, int ry)
{
    if (iCurr < 1 || d->state != NEIS_DECODE_FINISH)
        return;
    
    float zoom = iMgr->GetCurrentZoomFactor();

    TRect dst(rect->l, rect->t, rect->r, rect->b);
    if (dst.Width() == 0 || dst.Height() == 0)
        return;
    
    TRect src(0, 0, d->size.w, d->size.h);
    
    CFbsBitmap *bmp, *mask;
    TRect dd = dst, dt;
    
    CImageImplFrame* curFrame = iFrames[d->curr];
    if (fabs(zoom - 1.0) < NFLOAT_PRECISION) {
        ClearTempBitmaps();
        bmp = curFrame->iFrame;
        mask = curFrame->iMask;
    }
    else { // 缩放模式
        CreateTempBitmap(curFrame, bmp, mask, dst);
        src = TRect(TPoint(0, 0), bmp->SizeInPixels());
    }

    // 贴图
    int n = 0;
    
    // 单次
    if (rx == 0 && ry == 0) {
        n = 1;
        if (mask)
            gdi->BitBltMask(dd, bmp, src, mask, EFalse);
        else
            gdi->BitBlt(dd, bmp);
    }
    // 水平平铺
    else if (rx && ry == 0) {
        for (int i = 0; i < rx; i++) {
            dt = dd;
            n++;
            if (mask)
                gdi->BitBltMask(dt, bmp, src, mask, EFalse);
            else
                gdi->BitBlt(dt, bmp);
            dd.Move(dst.Width(), 0);
        }
    }
    // 垂直平铺
    else if (rx == 0 && ry) {
        for (int i = 0; i < ry && i < MAX_REPEAT_Y; i++) {
            dt = dd;
            n++;
            if (mask)
                gdi->BitBltMask(dt, bmp, src, mask, EFalse);
            else
                gdi->BitBlt(dt, bmp);
            dd.Move(0, dst.Height());
        }
    }
    // 水平、垂直平铺
    else {
        for (int i = 0; i < ry && i < MAX_REPEAT_Y; i++) {
            for (int j = 0; j < rx; j++) {
                dt = dd;
                n++;
                if (mask)
                    gdi->BitBltMask(dt, bmp, src, mask, EFalse);
                else
                    gdi->BitBlt(dt, bmp);
                dd.Move(dst.Width(), 0);
            }
            dd = dst;
            dd.Move(0, dst.Height() * (i + 1));
        }
    }
}

void CImageImpl::ClearFrames()
{
    d->curr = d->total = 0;
    
    for (int i=0; i < iFrames.Count(); i++)
        delete iFrames[i];
    iFrames.Close();
}

void CImageImpl::ClearDecoder()
{
    if (iDecoder) {
        iDecoder->Cancel();
        delete iDecoder;
        iDecoder = NULL;
    }
}


void CImageImpl::ClearDataStream()
{
    if (iSrcData) {
        NBK_free(iSrcData);
        iSrcData = NULL;
        iDataLen = 0;
    }
}

TBool CImageImpl::ParseOnePixelImag(CNbkGdi* aGdi, const TRect& aRect)
{    
    TInt maxSampling = 10; //最大采样次数
    TUint32 samplingValues[10]; //采样色值
    TInt samplingCnt; //实际采样次数

    TBitmapUtil bmpUtil(iFrames[0]->iFrame);
    TPoint pos;
    TInt xPos, yPos;

    TRect clipRect = aGdi->GetClippingRect();
    
    if (d->size.w == 1 && d->size.h > 1) {
        TInt yMin = clipRect.iTl.iY - aRect.iTl.iY;
        TInt yMax = clipRect.iBr.iY - aRect.iTl.iY;
        TInt offset = 1;
        if (clipRect.Width() > maxSampling)
            offset = clipRect.Width() / maxSampling;
        
        yPos = yMin;
        pos.SetXY(0, yPos);
        bmpUtil.Begin(pos, iFrames[0]->iFrame);

        for (samplingCnt = 0; samplingCnt < maxSampling; samplingCnt++) {
            yPos += offset;
            if (yPos > yMax)
                break;
            pos.SetXY(0, yPos);
            bmpUtil.SetPos(pos);
            samplingValues[samplingCnt] = bmpUtil.GetPixel();
        }
        bmpUtil.End();
    }
    else if (d->size.w > 1 && d->size.h == 1) {
        TInt xMin = clipRect.iTl.iX - aRect.iTl.iX;
        TInt xMax = clipRect.iBr.iX - aRect.iTl.iX;
        TInt offset = 1;
        if (clipRect.Width() > maxSampling)
            offset = clipRect.Width() / maxSampling;
        
        xPos = xMin;
        pos.SetXY(xPos, 0);
        bmpUtil.Begin(pos, iFrames[0]->iFrame);

        for (samplingCnt = 0; samplingCnt < maxSampling; samplingCnt++) {
            xPos += offset;
            if (xPos > xMax)
                break;
            pos.SetXY(xPos, 0);
            bmpUtil.SetPos(pos);
            samplingValues[samplingCnt] = bmpUtil.GetPixel();
        }
        bmpUtil.End();
    }
    else//1*1
    {
        samplingCnt = 1;
        pos.SetXY(0, 0);
        bmpUtil.Begin(pos, iFrames[0]->iFrame);
        samplingValues[0] = bmpUtil.GetPixel();
        bmpUtil.End();
    }
    
    if (samplingCnt == 1) {
        iColor = samplingValues[0];
        return ETrue;
    }
    else if (samplingCnt > 1 && samplingCnt < maxSampling) {
        TUint32 color;
        TBool sameColor = EFalse;
        for (TInt i = 0; i < samplingCnt - 1; i++) {
            if (samplingValues[i] == samplingValues[i + 1]) {
                color = samplingValues[i];
                sameColor = ETrue;
            }
            else {
                sameColor = EFalse;
                break;
            }
        }       
        if (sameColor)
            iColor = color;

        return sameColor;
    }
    else if (samplingCnt == maxSampling) {
        TUint32 color;
        TInt diffCnt = 0;
        TBool sameColor = EFalse;
        for (TInt i = 0; i < samplingCnt - 1; i++) {
            if (samplingValues[i] == samplingValues[i + 1]) {
                color = samplingValues[i];
                sameColor = ETrue;
            }
            else
                diffCnt++;
        }
        TInt maxDiffColorCnt = 2; //采样次数下最大不同RGB色，小于此值时近似为纯色
        if (sameColor && diffCnt <= maxDiffColorCnt) {
            iColor = color;
            return ETrue;
        }
    }   
    return EFalse;
}

TBool CImageImpl::Compare(const NImage* image)
{
    if (d->pageId == image->pageId && d->id == image->id)
        return ETrue;
    else
        return EFalse;
}

void CImageImpl::BuildAnimationFrameL()
{
    TDisplayMode dismode = iMgr->GetDisplayMode();

    RPointerArray<CImageImplFrame> finalFrameLst;
    for (TInt i = 0; i < iFrames.Count(); i++) {
        CImageImplFrame* fr = iFrames[i];

        TFrameInfo frameInfo(iDecoder->FrameInfo(i));

        CImageImplFrame* finalFrame = new (ELeave) CImageImplFrame;
        finalFrame->iFrame = new (ELeave) CFbsBitmap();
        finalFrame->iFrame->Create(iFrameInfo.iOverallSizeInPixels, dismode/*EColor16M*/);

        if (iFrameInfo.iFlags & TFrameInfo::ETransparencyPossible) {
            // we only support gray2 and gray256 tiling
            TDisplayMode maskmode = ((iFrameInfo.iFlags & TFrameInfo::EAlphaChannel)
                && (iFrameInfo.iFlags & TFrameInfo::ECanDither)) ? EGray256 : EGray2;

            finalFrame->iMask = new (ELeave) CFbsBitmap();
            finalFrame->iMask->Create(iFrameInfo.iOverallSizeInPixels, maskmode);
        }

        //If the first frame starts from position(0,0), copy directly to the destination bitmap 
        //otherwise frame has to be appropriately positioned in the destination bitmap  
        TPoint aStartPoint(0, 0);
        if ((i == 0) && (frameInfo.iFrameCoordsInPixels.iTl == aStartPoint)) {
            // First frame can be directly put into destination

            TInt bitmapHandle(fr->iFrame->Handle());
            if (bitmapHandle) {
                finalFrame->iFrame->Reset();
                finalFrame->iFrame->Duplicate(bitmapHandle);
            }

            if (finalFrame->iMask)
                finalFrame->iMask->Reset();

            if (fr->iMask && finalFrame->iMask) {
                finalFrame->iMask->Duplicate(fr->iMask->Handle());
            }
        }
        else {
            
            if (i > 0) {
                CImageImplFrame* preFrame = finalFrameLst[i - 1];

                CFbsBitmap& prevBitmap = *(preFrame->iFrame);
                CFbsBitmap& prevMask = *(preFrame->iMask);

                // Other frames must be build on top of previous frames
                if (!prevBitmap.Handle())
                    return;

                if (prevBitmap.Handle()) {
                    BitmapUtil::CopyBitmapData(*(preFrame->iFrame), *(finalFrame->iFrame),
                        preFrame->iFrame->SizeInPixels(), preFrame->iFrame->DisplayMode());
                }

                if (preFrame->iMask && finalFrame->iMask) {
                    BitmapUtil::CopyBitmapData(*(preFrame->iMask), *(finalFrame->iMask),
                        preFrame->iMask->SizeInPixels(), preFrame->iMask->DisplayMode());
                }
            }

            // Create bitmap device to destination bitmap
            CFbsBitGc* bitGc;
            CFbsBitmapDevice* bitDevice = CFbsBitmapDevice::NewL(finalFrame->iFrame);
            CleanupStack::PushL(bitDevice);
            User::LeaveIfError(bitDevice->CreateContext(bitGc));
            CleanupStack::PushL(bitGc);

            // Restore area in destination bitmap if needed
            TRect restoreRect;
            TBool restoreToBackground(EFalse);

            TInt aFrameNo = (i >= 1) ? (i) : 1;

            TFrameInfo prevFrameInfo(iDecoder->FrameInfo(aFrameNo - 1));

            if ((prevFrameInfo.iFlags & TFrameInfo::ERestoreToBackground) || i == 0) {
                restoreToBackground = ETrue;
                restoreRect = prevFrameInfo.iFrameCoordsInPixels;
                bitGc->SetPenColor(prevFrameInfo.iBackgroundColor);
                bitGc->SetBrushColor(prevFrameInfo.iBackgroundColor);
                bitGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
                if (i == 0) {
                    bitGc->Clear();
                }
                else {
                    bitGc->DrawRect(restoreRect);
                }
                bitGc->SetBrushStyle(CGraphicsContext::ENullBrush);
            }
            // Copy animation frame to destination bitmap
            if (fr->iMask && fr->iMask->Handle()) {
                bitGc->BitBltMasked(frameInfo.iFrameCoordsInPixels.iTl, fr->iFrame,
                    fr->iFrame->SizeInPixels(), fr->iMask, EFalse);
            }
            else {
                bitGc->BitBlt(frameInfo.iFrameCoordsInPixels.iTl, fr->iFrame,
                    fr->iFrame->SizeInPixels());
            }
            CleanupStack::PopAndDestroy(2); // bitmapCtx, bitmapDev

            // Combine masks if any
            if (finalFrame->iMask && fr->iMask) {
                bitDevice = CFbsBitmapDevice::NewL(finalFrame->iMask);
                CleanupStack::PushL(bitDevice);
                User::LeaveIfError(bitDevice->CreateContext(bitGc));
                CleanupStack::PushL(bitGc);

                if (restoreToBackground) {
                    bitGc->SetBrushColor(KRgbBlack);
                    bitGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
                    if (i == 0) {
                        bitGc->Clear();
                    }
                    else {
                        bitGc->DrawRect(restoreRect);
                    }
                    bitGc->SetBrushStyle(CGraphicsContext::ENullBrush);
                }
                CFbsBitmap* tmpMask = new (ELeave) CFbsBitmap;
                CleanupStack::PushL(tmpMask);
                User::LeaveIfError(tmpMask->Create(finalFrame->iMask->SizeInPixels(),
                    finalFrame->iMask->DisplayMode()));
                CFbsBitmapDevice* tmpMaskDev = CFbsBitmapDevice::NewL(tmpMask);
                CleanupStack::PushL(tmpMaskDev);
                CFbsBitGc* tmpMaskGc;
                User::LeaveIfError(tmpMaskDev->CreateContext(tmpMaskGc));
                CleanupStack::PushL(tmpMaskGc);

                tmpMaskGc->BitBlt(TPoint(0, 0), finalFrame->iMask, frameInfo.iFrameCoordsInPixels);

                bitGc->BitBltMasked(frameInfo.iFrameCoordsInPixels.iTl, fr->iMask,
                    fr->iMask->SizeInPixels(), tmpMask, ETrue);

                CleanupStack::PopAndDestroy(5); //tmpMask, tmpMaskDev, tmpMaskGc, bitGc, bitDevice
            }
        }

        finalFrameLst.AppendL(finalFrame);
    }
    
    iFrames.ResetAndDestroy();
    for (TInt i = 0; i < finalFrameLst.Count(); i++) {
        iFrames.AppendL(finalFrameLst[i]);
    }
    finalFrameLst.Reset();
}

// -----------------------------------------------------------------------------
// CMaskedBitmap::CopyBitmapData
// -----------------------------------------------------------------------------
TInt BitmapUtil::CopyBitmapData(const CFbsBitmap& aSource, CFbsBitmap& aDestination,
    const TSize& aSize, const TDisplayMode& aDisplayMode)
{
    // TODO - how to check if source or destination is null reference
    HBufC8* scanLine = HBufC8::New(aSource.ScanLineLength(aSize.iWidth, aDisplayMode));
    if (scanLine) {
        TPtr8 scanPtr(scanLine->Des());
        TPoint pp;
        for (pp.iY = 0; pp.iY < aSize.iHeight; ++pp.iY) {
            aSource.GetScanLine(scanPtr, pp, aSize.iWidth, aDisplayMode);
            aDestination.SetScanLine(scanPtr, pp.iY);
        }

        delete scanLine;
        return KErrNone;
    }
    return KErrNoMemory; // scanLine alloc failed
}

// -----------------------------------------------------------------------------
// 云简排特殊背景拉伸功能
// -----------------------------------------------------------------------------

// 判定是否为有效颜色（目前认为接近白色为无效色）
static bool is_valid_color(TRgb color)
{
    if (   color.Red() > 240
        && color.Green() > 240
        && color.Blue() > 240 )
        return false;
    else
        return true;
}

// 判断两个颜色是否明显不同
static bool is_differ_color(TRgb c1, TRgb c2)
{
    if (   Abs(c1.Red() - c2.Red()) > 30
        || Abs(c1.Green() - c2.Green()) > 30
        || Abs(c1.Blue() - c2.Blue()) > 30 )
        return true;
    else
        return false;
}

// 检测图片边框宽度（目前仅检测底边）
static bool guess_sides(const CFbsBitmap* aSrc, int& l, int& t, int& r, int& b)
{
    const int sideW = 2;
    int w = aSrc->SizeInPixels().iWidth;
    int h = aSrc->SizeInPixels().iHeight;
    bool ret = true;
    
    if (w < 6 || h < 6)
        return false;
    
    TPoint pos;
    TRgb color, last;
    int i, limit;
    bool found = false;
    
    l = t = r = sideW;

    limit = h - (int)(h * 0.3);
    for (i = h-1; i >= limit; i--) {
        pos.SetXY(w / 2, i);
        aSrc->GetPixel(color, pos);
        if (is_valid_color(color)) {
            if (!found) {
                found = true;
                last = color;
            }
            else if (is_differ_color(last, color))
                break;
        }
        else if (found)
            break;
    }

    if (i > limit)
        b = h - i;
    else
        ret = false;
    
    return ret;
}

#if 0
CFbsBitmap* CImageImpl::StretchBitmap(const CFbsBitmap* aSrc, const TSize& aSize, const TRect& aSrcR)
{
    CFbsBitmap* dst = new CFbsBitmap(); 
    CFbsBitmapDevice* dev;
    CFbsBitGc* gc;
    dst->Create(aSize, aSrc->DisplayMode());
    dev = CFbsBitmapDevice::NewL(dst);
    dev->CreateContext(gc);
    gc->DrawBitmap(TRect(TPoint(0, 0), aSize), aSrc, aSrcR);
    delete gc;
    delete dev;
    return dst;
}

CFbsBitmap* CImageImpl::StretchBitmapNinePalace(const CFbsBitmap* aSrc, const TSize& aSize, const TRect& aSrcR)
{
    const int sideW = 2;
    int sl, st, sr, sb;
    
    // 获取操作图
    CFbsBitmap* base = (CFbsBitmap*)aSrc;
    if (aSrcR.Size() != aSrc->SizeInPixels())
        base = StretchBitmap(aSrc, aSrcR.Size(), aSrcR);
    
    bool got = guess_sides(base, sl, st, sr, sb);
    
    if (   !got
        || aSrcR.Width() < sl + sr + sideW
        || aSrcR.Height() < st + sb + sideW
        || aSize.iWidth < sl + sr + sideW
        || aSize.iHeight < st + sb + sideW )
    {
        if (base != aSrc)
            delete base;
        return StretchBitmap(aSrc, aSize, aSrcR);
    }
    
    int ow = base->SizeInPixels().iWidth;
    int oh = base->SizeInPixels().iHeight;
    int w = aSize.iWidth;
    int h = aSize.iHeight;
    
    TRect dRect, sRect;
    CFbsBitmap* dst = new CFbsBitmap(); 
    CFbsBitmapDevice* dev;
    CFbsBitGc* gc;
    dst->Create(aSize, base->DisplayMode());
    dev = CFbsBitmapDevice::NewL(dst);
    dev->CreateContext(gc);
    
    // 左上角
    dRect.SetRect(0, 0, sl, st);
    sRect = dRect;
    gc->DrawBitmap(dRect, base, sRect);
    
    // 顶边
    dRect.SetRect(sl, 0, w - sr, st);
    sRect.SetRect(sl, 0, ow - sr, st);
    gc->DrawBitmap(dRect, base, sRect);

    // 右上角
    dRect.SetRect(w - sr, 0, w, st);
    sRect.SetRect(ow - sr, 0, ow, st);
    gc->DrawBitmap(dRect, base, sRect);
    
    // 左边
    dRect.SetRect(0, st, sl, h - sb);
    sRect.SetRect(0, st, sl, oh - sb);
    gc->DrawBitmap(dRect, base, sRect);
    
    // 中心
    dRect.SetRect(sl, st, w - sr, h - sb);
    sRect.SetRect(sl, st, ow - sr, oh - sb);
    gc->DrawBitmap(dRect, base, sRect);
    
    // 右边
    dRect.SetRect(w - sr, st, w, h - sb);
    sRect.SetRect(ow - sr, st, ow, oh - sb);
    gc->DrawBitmap(dRect, base, sRect);
    
    // 左下角
    dRect.SetRect(0, h - sb, sl, h);
    sRect.SetRect(0, oh - sb, sl, oh);
    gc->DrawBitmap(dRect, base, sRect);
    
    // 底边
    dRect.SetRect(sl, h - sb, w - sr, h);
    sRect.SetRect(sl, oh - sb, ow - sr, oh);
    gc->DrawBitmap(dRect, base, sRect);
    
    // 右下角
    dRect.SetRect(w - sr, h - sb, w, h);
    sRect.SetRect(ow - sr, oh - sb, ow, oh);
    gc->DrawBitmap(dRect, base, sRect);
    
    if (base != aSrc)
        delete base;
    delete gc;
    delete dev;
    return dst;
}

CFbsBitmap* CImageImpl::StretchIt(const CFbsBitmap* aSrc, const TSize& aSize, const TRect& aSrcR)
{
    return StretchBitmap(aSrc, aSize, aSrcR);
}

void CImageImpl::StretchFrames()
{
    // 创建缩放图
    CImageImplFrame* fr = iFrames[0];
    CImageImplFrame* zfr = new CImageImplFrame();
    TSize size(d->size.w, d->size.h);
    TSize srcSize(fr->iFrame->SizeInPixels());
    TRect srcRect(TPoint(0, 0), srcSize);
    coord x, y, w, h;
    
    if (d->sti && !d->cli && !d->stis /*&& (d->size.w != d->sti->w || d->size.h != d->sti->h)*/) {
        // 双向平铺图
        
        x = css_calcBgPos(d->sti->ow, srcSize.iWidth, d->sti->bg_x, N_TRUE);
        y = css_calcBgPos(d->sti->oh, srcSize.iHeight, d->sti->bg_y, N_TRUE);
        w = srcSize.iWidth;
        h = srcSize.iHeight;
        
        if (x < 0) {
            if (-x < w)
                x = -x;
            else
                (x + w < 0 || x + w >= w) ? x = 0 : x += w;
        }
        if (y < 0) {
            if (-y < h)
                y = -y;
            else
                (y + h < 0 || y + h >= h) ? y = 0 : y += h;
        }
        w = N_MIN(w, d->sti->ow);
        h = N_MIN(h, d->sti->oh);
        srcRect.SetRect(x, y, x+w, y+h);
        
        size.iWidth = (float)d->sti->w / d->sti->ow * srcSize.iWidth;
        size.iHeight = (float)d->sti->h / d->sti->oh * srcSize.iHeight;
        size.iWidth = (size.iWidth < 1) ? srcSize.iWidth : size.iWidth;
        size.iHeight = (size.iHeight < 1) ? srcSize.iHeight : size.iHeight;
        size.iWidth = (size.iWidth > d->sti->w) ? d->sti->w : size.iWidth;
        size.iHeight = (size.iHeight > d->sti->h) ? d->sti->h : size.iHeight;
        d->size.w = size.iWidth;
        d->size.h = size.iHeight;
        
        zfr->iFrame = StretchIt(fr->iFrame, size, srcRect);
        if (fr->iMask)
            zfr->iMask = StretchIt(fr->iMask, size, srcRect);
        d->sti->user = zfr;
    }
    else if (d->stis == N_NULL) {
        zfr->iFrame = StretchIt(fr->iFrame, size, srcRect);
        if (fr->iMask)
            zfr->iMask = StretchIt(fr->iMask, size, srcRect);
    }
    else {
        // 拉伸主图
        x = -css_calcBgPos(d->sti->ow, srcSize.iWidth, d->sti->bg_x, N_FALSE);
        y = -css_calcBgPos(d->sti->oh, srcSize.iHeight, d->sti->bg_y, N_FALSE);
        w = srcSize.iWidth - x;
        h = srcSize.iHeight - y;
        if (w > d->sti->ow) w = d->sti->ow;
        if (h > d->sti->oh) h = d->sti->oh;
        srcRect.SetRect(x, y, x + w, y + h);
        zfr->iFrame = StretchIt(fr->iFrame, size, srcRect);
        if (fr->iMask)
            zfr->iMask = StretchIt(fr->iMask, size, srcRect);
        d->sti->user = zfr;
    }
    
    // 拉伸所有切图
    NImStretchInfo* si = (d->stis) ? (NImStretchInfo*)sll_first(d->stis) : N_NULL;
    while (si) {
        CImageImplFrame* nfr = new CImageImplFrame();
        
        size.SetSize(si->w, si->h);

        x = -css_calcBgPos(si->ow, srcSize.iWidth, si->bg_x, N_FALSE);
        y = -css_calcBgPos(si->oh, srcSize.iHeight, si->bg_y, N_FALSE);
        w = srcSize.iWidth - x;
        h = srcSize.iHeight - y;
        if (w > si->ow) w = si->ow;
        if (h > si->oh) h = si->oh;
        srcRect.SetRect(x, y, x + w, y + h);
        
        nfr->iFrame = StretchIt(fr->iFrame, size, srcRect);
        if (fr->iMask)
            nfr->iMask = StretchIt(fr->iMask, size, srcRect);
        si->user = nfr;
        
        si = (NImStretchInfo*)sll_next(d->stis);
    }

    iFrames.ResetAndDestroy();
    iFrames.Append(zfr);
}
#endif
