/*
 * gdi.h
 *
 *  Created on: 2010-12-26
 *      Author: wuyulun
 */

#ifndef GDI_H_
#define GDI_H_

#include <e32std.h>
#include <gdi.h>

#define MAX_FONT_NUM    16
#define MAX_ZOOMFONT_NUM    128 
#define NFLOAT_PRECISION 0.01

class CNBrKernel;
class CProbe;

struct FontItem {
    TInt16  pixel; // 字体像素高
    TBool   bold; // 粗体
    TBool   italic; // 斜体
    TInt16  w; // 汉字均宽
    float   zoom; // 应用的缩放比例
    CFont*  font;
};

class CFontMgr
{
public:
    CFontMgr();
    virtual ~CFontMgr();
    
    void Reset();

    CFont* CreateFont(TInt16 aPixel, TBool aBold, TBool aItalic);

    // 获取字体，当字体不存在时创建字体
    TInt8 GetFont(TInt16 aPixel, TBool aBold, TBool aItalic, float aZoom);
    
    CFont* GetFont(TInt8 aFontId);
    CFont* GetFontByEditor(TInt8 aFontId);
    CFont* GetZoomFont(TInt8 aFontId, float aZoom, TBool& aLittleFont);
    
//    CFont* GetCacheFont(TInt16 aPixel, TBool aBold, TBool aItalic,float aZoom);

    // 获取汉字均宽
    TInt16 GetCharWidth(TInt8 aFontId) const;
    TInt16 GetCharWidthByEditor(TInt8 aFontId) const;

    void SetAntiAliasing(TBool aAnti) { iAnti = aAnti; }
    TBool IsAntiAliasing() const { return iAnti; }
    
    TInt16 ComputedPixelSize(TInt16 aPixel,float aZoom);
    CProbe* iProbe;
    
private:
    
    FontItem iFontList[MAX_FONT_NUM]; // 主字体表
    FontItem iZoomFontList[MAX_FONT_NUM]; // 与主字体表对应的缩放字体表  
//    FontItem iCachedZoomFontList[MAX_ZOOMFONT_NUM]; // 缓存缩放字体

    int iUse;
    TInt iZoomFontCnt;
    TBool iAnti;
};

class CNbkGdi
{
public:
    CProbe* iProbe;
    
public:
    CNbkGdi(CNBrKernel* aNbk);
    virtual ~CNbkGdi();
    
    void DrawText(const TDesC& aText, TPoint& aPos, const TInt aDecor);
    void DrawRect(TRect& aRect);
    void DrawCircle(TRect& aRect);
    void ClearRect(TRect& aRect);
  
    CFbsBitmap* GetTransparentBmpL(CFbsBitmap* aBitmap,const TUint8 aAlpha);
    
    // 绘制图片，可伸缩
    void DrawBitmap(TRect& aDestRect, const CFbsBitmap* aSource);
    void DrawBitmapMask(TRect& aDestRect, const CFbsBitmap* aBitmap,
                        TRect& aSourceRect, const CFbsBitmap* aMaskBitmap, TBool aInvertMask);

    // 绘制图片，不可伸缩
    void BitBlt(TRect& aDestRect, const CFbsBitmap* aSource);
    void BitBltMask(TRect& aDestRect, const CFbsBitmap* aBitmap,
                    TRect& aSourceRect, const CFbsBitmap* aMaskBitmap, TBool aInvertMask);
    
    void CopyRect(const TPoint& aOffset,const TRect& aRect);

    // 在不显示图片的情况下，绘制图片占位符
    void DrawBitmapPlaceholder(TRect& aRect, const TRgb aColor);
    
    TInt8 GetFont(TInt16 aPixel, TBool aBold, TBool aItalic);
    const CFont* GetFont(TInt8 aFontId);
    //返回字体，如字体存在，创建新字体并返回
    const CFont* GetZoomFont(TInt8 aFontId);
    //返回缓存字体，如字体不存在，返回NULL
//    CFont* GetCacheFont(TInt16 aPixel, TBool aBold, TBool aItalic);
    TInt GetZoomFontPixel(TInt16 aPixel);

    void UseFont(TInt8 aFontId, TBool aZoom = ETrue);
    void DiscardFont();
    TInt16 GetFontHeight(TInt8 aFontId);
    TInt16 GetFontAscent(TInt8 aFontId);
    TInt16 GetFontDescent(TInt8 aFontId);
    TInt16 GetCharWidth(TInt8 aFontId, const TChar ch);
    TInt16 GetTextWidth(TInt8 aFontId, const TPtrC aText);

    // 编辑器专用
    TInt16 GetCharWidthByEditor(TInt8 aFontId, const TChar ch);
    TInt16 GetTextWidthByEditor(TInt8 aFontId, const TPtrC text);
    TInt16 GetFontHeightByEditor(TInt8 aFontId);
    void DrawEditorCursor(TPoint& aPos, TInt aXOffset, TInt aCursorHeight);
    void DrawEditorScrollbar(TPoint& aPos, TInt aXOffset, TInt aYOffset, TSize& aSize);
    void DrawEditorText(const TDesC& aText, TPoint& aPos, TInt aXOffset);

    void SetBrushColor(const TRgb& aColor);
    void SetPenColor(const TRgb& aColor);
    
    void SetClippingRect(TRect& aRect);
    void CancelClipplingRect();
    TRect GetClippingRect(void) { return iUserClipRect; }
    
    TBool IsScaled() const;
    float ScalingFactor(void) const;

    void SetDrawOffset(const TPoint& aOffset) { iDrawOffset = aOffset; }
    TPoint GetDrawOffset(void) { return iDrawOffset; }
    void SetAntiAliasing(TBool aAnti);
    TBool IsAntiAliasing() const;
    
    TRect GetVisibleRect();
    
    CFontMgr* GetFontMgr() { return iFontMgr; }
    
private:
    TPoint GetBaselineOffset(void);
    CNBrKernel* iNbk;
    CFontMgr* iFontMgr;
    TBool iUsedLittleFont;
    TInt iFontId;
    
    TPoint iDrawOffset; // 绘制偏移量，用于局部绘制
    TRect iUserClipRect;
};

#endif /* GDI_H_ */
