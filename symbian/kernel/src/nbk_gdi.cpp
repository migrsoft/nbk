/*
 * nbk_gdi.cpp
 *
 *  Created on: 2010-12-26
 *      Author: wuyulun
 */

#include "../../../stdc/inc/nbk_gdi.h"
#include <e32std.h>
#include "NbkGdi.h"
#include "NBrKernel.h"
#include "NBKPlatformData.h"

void NBK_gdi_drawText(void* pfd, const wchr* text, int length, const NPoint* pos, const int decor)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TPtrC16 t(text, length);
    TPoint p(pos->x, pos->y);
    gdi->DrawText(t, p, decor);
}

void NBK_gdi_drawRect(void* pfd, const NRect* rect)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TRect r(rect->l, rect->t, rect->r, rect->b);
    gdi->DrawRect(r);
}

void NBK_gdi_drawEllipse(void* pfd, const NRect* rect)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TRect r(rect->l, rect->t, rect->r, rect->b);
    gdi->DrawCircle(r);
}

NFontId NBK_gdi_getFont(void* pfd, int16 pixel, nbool bold, nbool italic)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetFont(pixel, bold, italic);
}

void NBK_gdi_useFont(void* pfd, NFontId id)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    gdi->UseFont(id);
}

void NBK_gdi_useFontNoZoom(void* pfd, NFontId id)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    gdi->UseFont(id, EFalse);
}

void NBK_gdi_releaseFont(void* pfd)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    gdi->DiscardFont();
}

coord NBK_gdi_getFontHeight(void* pfd, NFontId id)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetFontHeight(id);
}

coord NBK_gdi_getCharWidth(void* pfd, NFontId id, const wchr ch)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetCharWidth(id, TChar(ch));
}

coord NBK_gdi_getTextWidth(void* pfd, NFontId id, const wchr* text, int length)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetTextWidth(id, TPtrC((TUint16*)text, length));
}

void NBK_gdi_setBrushColor(void* pfd, const NColor* color)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TUint r, g, b, a;
    r = color->r;
    g = color->g;
    b = color->b;
    a = color->a;
    gdi->SetBrushColor(TRgb((TInt)r, (TInt)g, (TInt)b, (int)a));
}

void NBK_gdi_fillRect(void* pfd, const NRect* rect)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TRect r(rect->l, rect->t, rect->r, rect->b);
    gdi->ClearRect(r);
}

void NBK_gdi_setPenColor(void* pfd, const NColor* color)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TUint r, g, b;
    r = color->r;
    g = color->g;
    b = color->b;
    gdi->SetPenColor(TRgb((TInt)r, (TInt)g, (TInt)b));
}

void NBK_gdi_setClippingRect(void* pfd, const NRect* rect)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TRect r(rect->l , rect->t, rect->r, rect->b);
    gdi->SetClippingRect(r);
}

void NBK_gdi_cancelClippingRect(void* pfd)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    gdi->CancelClipplingRect();
}

// 获取可视区域，左上角为文档坐标，宽高为屏幕坐标
void NBK_helper_getViewableRect(void* pfd, NRect* view)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* nbk = (CNBrKernel*)p->nbk;
    nbk->GetViewPort(view);
}

coord NBK_gdi_getTextWidthByEditor(void* pfd, NFontId id, const wchr* text, int length)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetTextWidthByEditor(id, TPtrC((TUint16*)text, length));
}

coord NBK_gdi_getFontHeightByEditor(void* pfd, NFontId id)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetFontHeightByEditor(id);
}

coord NBK_gdi_getCharWidthByEditor(void* pfd, NFontId id, const wchr ch)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    return gdi->GetCharWidthByEditor(id, TChar(ch));
}

void NBK_gdi_drawEditorCursor(void* pfd, NPoint* pt, coord xOffset, coord cursorHeight)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TPoint pos(pt->x, pt->y);
    gdi->DrawEditorCursor(pos, xOffset, cursorHeight);
}

void NBK_gdi_drawEditorScrollbar(void* pfd, NPoint* pt, coord xOffset, coord yOffset, NSize* size)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TPoint pos(pt->x, pt->y);
    TSize sz(size->w, size->h);
    gdi->DrawEditorScrollbar(pos, xOffset, yOffset, sz);
}

void NBK_gdi_drawEditorText(void* pfd, const wchr* text, int length, const NPoint* pos, coord xOffset)
{
    CNbkGdi* gdi = (CNbkGdi*)((NBK_PlatformData*)pfd)->gdi;
    TPtrC16 t(text, length);
    TPoint p(pos->x, pos->y);
    gdi->DrawEditorText(t, p, xOffset);
}
