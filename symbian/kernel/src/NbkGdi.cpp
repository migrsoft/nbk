/*
 * gdi.cpp
 *
 *  Created on: 2010-12-26
 *      Author: wuyulun
 */

#include "../../../stdc/inc/config.h"
#include "NbkGdi.h"
#include "NBrKernel.h"
#include "probe.h"
#include "ResourceManager.h"
#include <gdi.h>
#include <eikenv.h>
#include <w32std.h>
#include <math.h>
#include <aknfontaccess.h>

const int KMinAvailFontPixel = 7;

#define CACHE_ALL_ZOOM_FONT 0

//////////////////////////////////////////////////
//
// CFontMgr 字体管理器
//

CFontMgr::CFontMgr() :
    iZoomFontCnt(0)
{
    iAnti = EFalse;
    
    NBK_memset(iFontList, 0, sizeof(FontItem) * MAX_FONT_NUM);
    NBK_memset(iZoomFontList, 0, sizeof(FontItem) * MAX_FONT_NUM);
    
#if CACHE_ALL_ZOOM_FONT
    NBK_memset(iCachedZoomFontList, 0, sizeof(FontItem) * MAX_ZOOMFONT_NUM);
#endif
    iUse = 0;
}

CFontMgr::~CFontMgr()
{
    for (int i = 0; i < iUse; i++) {
        CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iFontList[i].font);
#if !CACHE_ALL_ZOOM_FONT
        if (iZoomFontList[i].font)
            CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iZoomFontList[i].font);
#endif
    }
    
#if CACHE_ALL_ZOOM_FONT
    for (int n = 0; n < iZoomFontCnt; n++) {
        CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iCachedZoomFontList[n].font);
    }
#endif
}

void CFontMgr::Reset()
{
//    iProbe->OutputChar("--- reset font ---", -1);
//    iProbe->OutputReturn();
    
    // 第一个字体保留
    for (int i = 1; i < iUse; i++) {
        CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iFontList[i].font);
        iFontList[i].font = NULL;
        if (iZoomFontList[i].font) {
            CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iZoomFontList[i].font);
            iZoomFontList[i].font = NULL;
        }
    }
    iUse = 1;
}

CFont* CFontMgr::CreateFont(TInt16 aPixel, TBool aBold, TBool aItalic)
{
    TFontSpec fontSpec;
    CFont* font = NULL;
    
    fontSpec = CEikonEnv::Static()->LegendFont()->FontSpecInTwips();
    fontSpec.iFontStyle.SetStrokeWeight(aBold ? EStrokeWeightBold : EStrokeWeightNormal);
    fontSpec.iFontStyle.SetPosture(aItalic ? EPostureItalic : EPostureUpright);
    fontSpec.iFontStyle.SetBitmapType(iAnti ? EAntiAliasedGlyphBitmap : EDefaultGlyphBitmap);
    fontSpec.iHeight = CEikonEnv::Static()->ScreenDevice()->VerticalPixelsToTwips(aPixel);
    CEikonEnv::Static()->ScreenDevice()->GetNearestFontInTwips(font, fontSpec);
    
    return font;
}

TInt8 CFontMgr::GetFont(TInt16 aPixel, TBool aBold, TBool aItalic, float aZoom)
{
    int i, idx = 0;
    
    // 在主字体表中查找
    for (i=0; i < iUse; i++) {
        if (   iFontList[i].pixel == aPixel
            && iFontList[i].bold == aBold
            && iFontList[i].italic == aItalic) {
            idx = i + 1;
            break;
        }
    }

    if (idx == 0) {
        if (iUse >= MAX_FONT_NUM) {
            // 超过字体数上限，默认返回第一个字体
            idx = 1;
        }
        else {
            // 创建内核指定字体
            CFont* font = CreateFont(aPixel, aBold, aItalic);
            iFontList[iUse].pixel = aPixel;
            iFontList[iUse].bold = aBold;
            iFontList[iUse].italic = aItalic;
            iFontList[iUse].zoom = KNbkDefaultZoomLevel;
            iFontList[iUse].font = font;
            iFontList[iUse].w = font->CharWidthInPixels(0x6c49); // 取“汉”字宽
            iUse++;
            idx = iUse;
            
//            iProbe->OutputChar("create font", -1);
//            iProbe->OutputInt(idx);
//            iProbe->OutputInt(aPixel);
//            iProbe->OutputChar("    height = ", -1);
//            iProbe->OutputInt((TInt16) font->FontMaxHeight());
//            
//            iProbe->OutputChar("   max descent = ", -1);
//            iProbe->OutputInt((TInt16) font->FontMaxDescent());
//            iProbe->OutputChar("   descent = ", -1);
//            iProbe->OutputInt((TInt16) font->DescentInPixels());
//            iProbe->OutputInt(aBold);
//            iProbe->OutputInt(aItalic);
//            iProbe->OutputReturn();
        }
    }
    
    if (fabs(aZoom - KNbkDefaultZoomLevel) >= NFLOAT_PRECISION) {
 
        // 查找缓存缩放字体
        TInt16 fontPixel = ComputedPixelSize(aPixel, aZoom);
        
        i = idx - 1;
        if (iZoomFontList[i].font) {
            // 对应缩放字体存在，检测参数
            if (   iZoomFontList[i].pixel == fontPixel
                && iZoomFontList[i].bold == aBold
                && iZoomFontList[i].italic == aItalic
                && iZoomFontList[i].zoom == aZoom)
                return idx;
            
#if !CACHE_ALL_ZOOM_FONT
            // 该字体无效，删除
            CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iZoomFontList[i].font);
#endif
        }
        
#if CACHE_ALL_ZOOM_FONT
        // 查找字体是否存在于字体缓存表
        for (TInt n = 0; n < iZoomFontCnt; n++) {
            if (   iCachedZoomFontList[n].pixel == fontPixel
                && iCachedZoomFontList[n].bold == aBold
                && iCachedZoomFontList[n].italic == aItalic) {
                
                iZoomFontList[i] = iCachedZoomFontList[n];
                iZoomFontList[i].zoom = aZoom;
                return idx;
            }
        }
        
        if (iZoomFontCnt >= MAX_ZOOMFONT_NUM) { // fixme: 是否总字体使用量有限
            idx = 1;
            return idx;
        }
#endif

        CFont* font = CreateFont(fontPixel, aBold, aItalic);
        
        FontItem fi;
        fi.pixel = fontPixel;
        fi.bold = aBold;
        fi.italic = aItalic;
        fi.zoom = aZoom;
        fi.font = font;
        fi.w = font->CharWidthInPixels(0x6c49);
        
        iZoomFontList[i] = fi;
        
#if CACHE_ALL_ZOOM_FONT
        iCachedZoomFontList[iZoomFontCnt++] = fi;
#endif
    }

    return idx;
}

CFont* CFontMgr::GetFont(TInt8 aFontId)
{
    if (aFontId >= 1 && aFontId <= iUse)
        return iFontList[aFontId - 1].font;
    return NULL;
}

CFont* CFontMgr::GetFontByEditor(TInt8 aFontId)
{
    if (aFontId >= 1 && aFontId <= iUse)
        return iZoomFontList[aFontId - 1].font;
    return NULL;
}

CFont* CFontMgr::GetZoomFont(TInt8 aFontId, float aZoom, TBool& aLittleFont)
{
    int i = aFontId - 1;
    TInt16 pixel = -1;
    TBool bold, italic;
    
    if (i >= 0 && i < iUse) {
        pixel = iFontList[i].pixel;
        bold = iFontList[i].bold;
        italic = iFontList[i].italic;
    }
    
    if (pixel == -1) { // 字体不存在
        aLittleFont = ETrue;
        return N_NULL;
    }
    
    TInt16 fh = ComputedPixelSize(pixel, aZoom);
    
    if (fh >= KMinAvailFontPixel) {
        aLittleFont = EFalse;
        i = GetFont(pixel, bold, italic, aZoom);
        return iZoomFontList[i - 1].font;
    }
    else {
        aLittleFont = ETrue;
        return NULL;
    }
}

//CFont* CFontMgr::GetCacheFont(TInt16 aPixel, TBool aBold, TBool aItalic, float aZoom)
//{
//    if (fabs(aZoom - KNbkDefaultZoomLevel) < NFLOAT_PRECISION) {
//
//        // 在主字体表中查找
//        for (TInt i = 0; i < iUse; i++) {
//            if (iFontList[i].pixel == aPixel && iFontList[i].bold == aBold && iFontList[i].italic
//                == aItalic)
//                return iFontList[i].font;
//        }
//    }
//    else {
//        // 查找缓存缩放字体
//        for (TInt n = 0; n < iZoomFontCnt; n++) {
//            if (iCachedZoomFontList[n].pixel == aPixel && iCachedZoomFontList[n].bold == aBold
//                && iCachedZoomFontList[n].italic == aItalic)
//                return iCachedZoomFontList[n].font;
//        }
//    }
//    
//    return NULL;
//}

TInt16 CFontMgr::GetCharWidth(TInt8 aFontId) const
{
    return iFontList[aFontId - 1].w;
}

TInt16 CFontMgr::GetCharWidthByEditor(TInt8 aFontId) const
{
    return iZoomFontList[aFontId - 1].w;
}

TInt16 CFontMgr::ComputedPixelSize(TInt16 aPixel, float aZoom)
{
    TInt16 fh = (TInt16) ((float) aPixel * aZoom /*+ 0.3f*/);
    return ((fh > 16) ? fh - 1 : fh);
}

//////////////////////////////////////////////////
//
// CNbkGdi GDI实现
//

CNbkGdi::CNbkGdi(CNBrKernel* aNbk)
    : iNbk(aNbk)
    , iUsedLittleFont(EFalse)
{
    iFontMgr = new CFontMgr();
}

CNbkGdi::~CNbkGdi()
{
    delete iFontMgr;
}

// 绘制矩形外框
void CNbkGdi::DrawRect(TRect& aRect)
{
    TRect r(aRect);
    r.Move(iDrawOffset);
    iNbk->iScreenGc->DrawRect(r);
}

void CNbkGdi::ClearRect(TRect& aRect)
{
    TRect r(aRect);
    r.Move(iDrawOffset);
    iNbk->iScreenGc->Clear(r);
}

CFbsBitmap* CNbkGdi::GetTransparentBmpL(CFbsBitmap* aBitmap,const TUint8 aAlpha)
{
    if (!aBitmap)
        return NULL;

    CFbsBitmapDevice* dev;
    CFbsBitGc* gc;
    TSize size = aBitmap->SizeInPixels();
    
    CFbsBitmap* transBmpMask = new (ELeave) CFbsBitmap();
    transBmpMask->Create(size, EGray256);
    dev = CFbsBitmapDevice::NewL(transBmpMask);
    CleanupStack::PushL(dev);
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::Pop();
    gc->SetBrushColor(TRgb::Gray256(aAlpha));
    gc->SetBrushStyle(CBitmapContext::ESolidBrush);
    gc->Clear();
    delete gc;
    delete dev;

    CFbsBitmap* res = new (ELeave) CFbsBitmap();
    res->Create(size, aBitmap->DisplayMode());
    dev = CFbsBitmapDevice::NewL(res);
    CleanupStack::PushL(dev);
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::Pop();
    TRect r(TPoint(0,0),size);
    gc->BitBltMasked(TPoint(0,0),aBitmap,r,transBmpMask,ETrue);
    delete gc;
    delete dev;
    delete transBmpMask;
    
    return res;
}

// aDecor: 文本装饰属性（0:无 1:下划线）
void CNbkGdi::DrawText(const TDesC& aText, TPoint& aPos, const TInt aDecor)
{
    if (iUsedLittleFont) {
        CFont* font = iFontMgr->GetFont(iFontId);
        float zoom = ScalingFactor();

        TRgb penColor = iNbk->iScreenGc->PenColor();
        TRgb oldColor = iNbk->iScreenGc->BrushColor();
        TRgb color;
        TRect r(0,0,0,0);
        int i, alpha, dx, w;
        for (i = dx = 0; i < aText.Length(); i++) {
            alpha = aText[i] % 8 * 8 + 64;
            color = penColor;
            color.SetAlpha(alpha);
            iNbk->iScreenGc->SetBrushColor(color);
            w = (int)(GetCharWidth(iFontId, aText[i]) * zoom);
            r.iTl.iX = aPos.iX + dx;
            r.iTl.iY = aPos.iY;
            r.SetWidth(w);
            r.iTl.iY -= 2;
            r.SetHeight(2);
            r.Move(iDrawOffset);
            iNbk->iScreenGc->Clear(r);
            dx += w;
        }
        iNbk->iScreenGc->SetBrushColor(oldColor);
    }
    else {
        aPos += iDrawOffset;
        if (aDecor == 1)
            iNbk->iScreenGc->SetUnderlineStyle(EUnderlineOn);
        
        aPos += GetBaselineOffset();
        iNbk->iScreenGc->DrawText(aText, aPos);
        if (aDecor == 1)
            iNbk->iScreenGc->SetUnderlineStyle(EUnderlineOff);
    }
}

// 画圆形
void CNbkGdi::DrawCircle(TRect& aRect)
{
    aRect.Move(iDrawOffset);
    iNbk->iScreenGc->DrawEllipse(aRect);
}

// 接收来自内核的字体请求
TInt8 CNbkGdi::GetFont(TInt16 aPixel, TBool aBold, TBool aItalic)
{
    iFontMgr->iProbe = iProbe;
    return iFontMgr->GetFont(aPixel, aBold, aItalic, 1.0);
}

const CFont* CNbkGdi::GetFont(TInt8 aFontId)
{
    return iFontMgr->GetFont(aFontId);
}

const CFont* CNbkGdi::GetZoomFont(TInt8 aFontId)
{
    float zoom = ScalingFactor();
    TBool little;
    
    if (fabs(zoom - KNbkDefaultZoomLevel) < NFLOAT_PRECISION)
        return iFontMgr->GetFont(aFontId);
    else
        return iFontMgr->GetZoomFont(aFontId, zoom, little);
}

//CFont* CNbkGdi::GetCacheFont(TInt16 aPixel, TBool aBold, TBool aItalic)
//{
//    float zoom = iNbk->GetCurrentZoomFactor();
//    return iFontMgr->GetCacheFont(aPixel, aBold, aItalic, zoom);
//}

TInt CNbkGdi::GetZoomFontPixel(TInt16 aPixel)
{
    float zoom = ScalingFactor();
    return iFontMgr->ComputedPixelSize(aPixel, zoom);
}

void CNbkGdi::UseFont(TInt8 aFontId, TBool aZoom)
{
    CFont* font = NULL;
    float zoom = ScalingFactor();

    iUsedLittleFont = EFalse;
    iFontId = aFontId;

    if (aZoom && fabs(zoom - KNbkDefaultZoomLevel) >= NFLOAT_PRECISION)
        font = iFontMgr->GetZoomFont(aFontId, zoom, iUsedLittleFont);
    else
        font = iFontMgr->GetFont(aFontId);
    
    if (!iUsedLittleFont) {
        iNbk->iScreenGc->UseFont(font);
    }
}

void CNbkGdi::DiscardFont()
{
    if (!iUsedLittleFont)
        iNbk->iScreenGc->DiscardFont();
    iUsedLittleFont = EFalse;
}

TInt16 CNbkGdi::GetFontHeight(TInt8 aFontId)
{
    CFont* font = iFontMgr->GetFont(aFontId);
    return (TInt16) font->FontMaxHeight();
}

TInt16 CNbkGdi::GetFontAscent(TInt8 aFontId)
{
    CFont* font = iFontMgr->GetFont(aFontId);
    TInt16 ascent = (TInt16)font->FontMaxAscent();
    if (!ascent) {
        ascent = (TInt16) font->HeightInPixels() * 7 / 10;
    }
    return ascent;    
}

TInt16 CNbkGdi::GetFontDescent(TInt8 aFontId)
{
    CFont* font = iFontMgr->GetFont(aFontId);
    TInt16 descent = (TInt16) font->FontMaxDescent();
    return descent;
}

TInt16 CNbkGdi::GetCharWidth(TInt8 aFontId, const TChar ch)
{
    if ((ch >= 0x3400 && ch <= 0x9fff) || (ch >= 0xf900 && ch <= 0xfaff))
        return iFontMgr->GetCharWidth(aFontId);
    else {
        CFont* font = iFontMgr->GetFont(aFontId);
        return (TInt16) font->CharWidthInPixels(ch);
    }
}

TInt16 CNbkGdi::GetTextWidth(TInt8 aFontId, const TPtrC aText)
{
    TInt16 width = 0;
    
    int len = aText.Length();
    for (int i = 0; i < len; i++) {
        width += GetCharWidth((iUsedLittleFont) ? iFontId : aFontId, TChar(aText[i]));
    }
    return width;
}

TInt16 CNbkGdi::GetCharWidthByEditor(TInt8 aFontId, const TChar ch)
{
    if (fabs(ScalingFactor() - KNbkDefaultZoomLevel) < NFLOAT_PRECISION)
        return GetCharWidth(aFontId, ch);
    
    CFont* font = iFontMgr->GetFontByEditor(aFontId);
    
    if (font == NULL)
        return 2;
    
    if ((ch >= 0x3400 && ch <= 0x9fff) || (ch >= 0xf900 && ch <= 0xfaff))
        return iFontMgr->GetCharWidthByEditor(aFontId);
    else
        return (TInt16) font->CharWidthInPixels(ch);
}

TInt16 CNbkGdi::GetTextWidthByEditor(TInt8 aFontId, const TPtrC aText)
{
    if (fabs(ScalingFactor() - KNbkDefaultZoomLevel) < NFLOAT_PRECISION)
        return GetTextWidth(aFontId, aText);
    
    TInt16 width = 0;
    
    int len = aText.Length();
    for (int i = 0; i < len; i++) {
        width += GetCharWidthByEditor((iUsedLittleFont) ? iFontId : aFontId, TChar(aText[i]));
    }
    return width;
}

TInt16 CNbkGdi::GetFontHeightByEditor(TInt8 aFontId)
{
    if (fabs(ScalingFactor() - KNbkDefaultZoomLevel) < NFLOAT_PRECISION)
        return GetFontHeight(aFontId);
    
    CFont* font = iFontMgr->GetFontByEditor(aFontId);
    
    if (font == NULL)
        return 2;
    
    return (TInt16) font->FontMaxHeight();
}

void CNbkGdi::SetBrushColor(const TRgb& aColor)
{
    iNbk->iScreenGc->SetBrushColor(aColor);
}

void CNbkGdi::DrawEditorCursor(TPoint& aPos, TInt aXOffset, TInt aCursorHeight)
{
    aPos += iDrawOffset;
    aPos.iX += aXOffset;
    
    TRect r(aPos, TSize(2, aCursorHeight));
    iNbk->iScreenGc->Clear(r);
}

void CNbkGdi::DrawEditorScrollbar(TPoint& aPos, TInt aXOffset, TInt aYOffset, TSize& aSize)
{
    aPos += iDrawOffset;
    aPos.iX += aXOffset;
    aPos.iY += aYOffset;
    
    TRect r(aPos, aSize);
    iNbk->iScreenGc->Clear(r);
}

void CNbkGdi::DrawEditorText(const TDesC& aText, TPoint& aPos, TInt aXOffset)
{
    if (iUsedLittleFont) {
    }
    else {
        aPos += iDrawOffset;
        aPos.iX += aXOffset;

        aPos += GetBaselineOffset();
        iNbk->iScreenGc->DrawText(aText, aPos);
    }
}

void CNbkGdi::SetPenColor(const TRgb& aColor)
{
    iNbk->iScreenGc->SetPenColor(aColor);
}

void CNbkGdi::DrawBitmap(TRect& aDestRect, const CFbsBitmap* aSource)
{
    aDestRect.Move(iDrawOffset);
    iNbk->iScreenGc->DrawBitmap(aDestRect, aSource);
}

void CNbkGdi::DrawBitmapMask(
    TRect& aDestRect, const CFbsBitmap* aBitmap,
    TRect& aSourceRect, const CFbsBitmap* aMaskBitmap, TBool aInvertMask)
{
    aDestRect.Move(iDrawOffset);
    iNbk->iScreenGc->DrawBitmapMasked(aDestRect, aBitmap, aSourceRect, aMaskBitmap, aInvertMask);
}

void CNbkGdi::BitBlt(TRect& aDestRect, const CFbsBitmap* aSource)
{
    aDestRect.Move(iDrawOffset);    
    iNbk->iScreenGc->BitBlt(aDestRect.iTl, aSource);
}

void CNbkGdi::BitBltMask(
    TRect& aDestRect, const CFbsBitmap* aBitmap,
    TRect& aSourceRect, const CFbsBitmap* aMaskBitmap, TBool aInvertMask)
{
    aDestRect.Move(iDrawOffset);
    iNbk->iScreenGc->BitBltMasked(aDestRect.iTl, aBitmap, aSourceRect, aMaskBitmap, aInvertMask);
}

void CNbkGdi::CopyRect(const TPoint& aOffset,const TRect& aRect)
{
    TRect tmp(aRect);
    tmp.Move(iDrawOffset);
    iNbk->iScreenGc->CopyRect(aOffset, tmp);
}

void CNbkGdi::DrawBitmapPlaceholder(TRect& aRect, const TRgb aColor)
{
    aRect.Move(iDrawOffset);

    // 铺底色
    TRgb color = iNbk->iScreenGc->BrushColor();
    iNbk->iScreenGc->SetBrushColor(aColor);
    iNbk->iScreenGc->Clear(aRect);
    iNbk->iScreenGc->SetBrushColor(color);
    
    // 画边框
    color = iNbk->iScreenGc->PenColor();
    iNbk->iScreenGc->SetPenColor(KRgbGray);
    iNbk->iScreenGc->DrawRect(aRect);
    iNbk->iScreenGc->SetPenColor(color);
}

void CNbkGdi::SetClippingRect(TRect& aRect)
{
    iUserClipRect = aRect;
    aRect.Move(iDrawOffset);
    iNbk->iScreenGc->SetClippingRect(aRect);
}

void CNbkGdi::CancelClipplingRect()
{
    iNbk->iScreenGc->CancelClippingRect();
}

TBool CNbkGdi::IsScaled() const
{
    if(fabs(ScalingFactor() - KNbkDefaultZoomLevel) < NFLOAT_PRECISION)
        return EFalse;
    else
        return ETrue;
}

float CNbkGdi::ScalingFactor(void) const
{
    return iNbk->GetCurZoom();
}

void CNbkGdi::SetAntiAliasing(TBool aAnti)
{
    iFontMgr->SetAntiAliasing(aAnti);
}

TBool CNbkGdi::IsAntiAliasing() const
{
    return iFontMgr->IsAntiAliasing();
}

TRect CNbkGdi::GetVisibleRect()
{
    NRect r;
    iNbk->GetViewPort(&r);
    TRect rect;
    rect.SetRect(iDrawOffset.iX, iDrawOffset.iY, r.r, r.b);
    return rect;
}

TPoint CNbkGdi::GetBaselineOffset(void)
{
    TPoint baselineOffset(0, 0);
    CFont* font = NULL;
    
    font = iFontMgr->GetFont(iFontId);
    if (font != NULL) {
        TInt ascent = font->FontMaxAscent();
        TInt descent = font->FontMaxDescent();
        font->FontMaxHeight();
        float zoom = ScalingFactor();
        ascent = (coord)float_imul(ascent, zoom);
        descent = (coord) float_imul(descent, zoom) / 2;
        baselineOffset.SetXY(0, ascent - descent);
    }

    return baselineOffset;
}

