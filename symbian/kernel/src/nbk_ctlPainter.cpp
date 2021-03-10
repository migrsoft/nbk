#include "../../../stdc/inc/nbk_ctlPainter.h"
#include "../../../stdc/tools/str.h"
#include "../../../stdc/tools/unicode.h"
#include "../../../stdc/inc/nbk_settings.h"
#include "NBKPlatformData.h"
#include "NBrKernel.h"
#include "NbkGdi.h"
#include "FEPHandler.h"
#include "ResourceManager.h"

// 绘制单行文本编辑器外框
nbool NBK_paintText(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{    
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    CNbkGdi* gdi = (CNbkGdi*)p->gdi;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;
    
	TRect r(rect->l, rect->t, rect->r, rect->b);
	TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
	MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;
	    
	if (cpi && cpi->OnInputTextPaint(k->GetScreenGc(), r, st, c))
		return N_TRUE;
	
	return N_FALSE;
}

// 绘制复选钮
nbool NBK_paintCheckbox(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    CNbkGdi* gdi = (CNbkGdi*)p->gdi;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;
    
    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
	MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;
	
    TRect focusR;
    k->GetFocusRect(k->GetFocus(), focusR);
    
    NRect viewPort;
    k->GetViewPort(&viewPort);
    focusR.Move(TPoint(-viewPort.l, -viewPort.t));
    
    if (focusR.Intersects(r)) {
        if (MNbkCtrlPaintInterface::EPressed == k->CurNodeState())
            st = MNbkCtrlPaintInterface::EPressed;
    }
    
	if (cpi && cpi->OnCheckBoxPaint(k->GetScreenGc(), r, st, (TBool)checked, c))
		return N_TRUE;
	
	return N_FALSE;	
}

// 绘制单选钮
nbool NBK_paintRadio(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    CNbkGdi* gdi = (CNbkGdi*)p->gdi;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;
    
    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
	MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;
	
    TRect focusR;
    k->GetFocusRect(k->GetFocus(), focusR);
    
    NRect viewPort;
    k->GetViewPort(&viewPort);
    focusR.Move(TPoint(-viewPort.l, -viewPort.t));
    
    if (focusR.Intersects(r)) {
        if (MNbkCtrlPaintInterface::EPressed == k->CurNodeState())
            st = MNbkCtrlPaintInterface::EPressed;
    }
    
	if (cpi && cpi->OnRadioPaint(k->GetScreenGc(), r, st, (TBool)checked, c))
		return N_TRUE;
	
	return N_FALSE;		
}

// 绘制按钮
nbool NBK_paintButton(
    void* pfd, const NFontId fontId, const NRect* rect,
    const wchr* text, int len, NECtrlState state, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
	CNBrKernel* k = (CNBrKernel*)p->nbk;
	CNbkGdi* gdi = (CNbkGdi*)p->gdi;
	MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;
	
    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
    
	TPtrC label;
	if (text)
	    label.Set(text, len);
	else
	    label.Set(KNullDesC);
    MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;
    
    const CFont* font = gdi->GetZoomFont(fontId);
    
    TRect focusR;
    k->GetFocusRect(k->GetFocus(), focusR);
    
    NRect viewPort;
    k->GetViewPort(&viewPort);
    focusR.Move(TPoint(-viewPort.l, -viewPort.t));
    
    if (focusR.Intersects(r)) {
        if (MNbkCtrlPaintInterface::EPressed == k->CurNodeState())
            st = MNbkCtrlPaintInterface::EPressed;
    }
    
    TBool ret = (cpi && cpi->OnButtonPaint(k->GetScreenGc(), font, r, label, st, c));
    return ret;	
}

// 绘制列表选择控件，未展开状态
nbool NBK_paintSelectNormal(
    void* pfd, const NFontId fontId, const NRect* rect,
    const wchr* text, int len, NECtrlState state, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
	CNBrKernel* k = (CNBrKernel*)p->nbk;
    CNbkGdi* gdi = (CNbkGdi*)p->gdi;
	MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;
	
    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
	
	TPtrC label(text, len);
	MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;
	
    const CFont* font = gdi->GetZoomFont(fontId);
    
    TRect focusR;
    k->GetFocusRect(k->GetFocus(), focusR);
    
    NRect viewPort;
    k->GetViewPort(&viewPort);
    focusR.Move(TPoint(-viewPort.l, -viewPort.t));
    
    if (focusR.Intersects(r)) {
        if (MNbkCtrlPaintInterface::EPressed == k->CurNodeState())
            st = MNbkCtrlPaintInterface::EPressed;
    }
    
	if (cpi && cpi->OnSelectNormalPaint(k->GetScreenGc(), font, r, label, st, c))
		return N_TRUE;
	
	return N_FALSE;
}

// 请求框架列表选择对话框
nbool NBK_paintSelectExpand(void* pfd, const wchr* items, int num, int* sel)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*) p->ctlPainter;
    RArray<TPtrC> options;
    wchr** lst = (wchr**)items;
    int i;
    nbool ret = N_FALSE;
    
    for (i=0; i < num; i++) {
        TPtrC it(KNullDesC16);
        if (lst[i])
            it.Set((TUint16*)lst[i], nbk_wcslen(lst[i]));
        options.Append(it);
    }

    // 模态对话框实现不正确时，阻止内核处理任何外部事件
    k->DenyEvent(ETrue);
    if (cpi && cpi->OnSelectExpandPaint(options, *sel))
        ret = N_TRUE;
    k->DenyEvent(EFalse);

    options.Close();
    
    return ret;
}

// 绘制多行编辑区外边框
nbool NBK_paintTextarea(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{  
    NBK_PlatformData* p = (NBK_PlatformData*) pfd;
    CNBrKernel* k = (CNBrKernel*) p->nbk;
    CNbkGdi* gdi = (CNbkGdi*) p->gdi;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*) p->ctlPainter;

    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
	MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;

	if (cpi && cpi->OnTextAreaPaint(k->GetScreenGc(), r, st, c))
		return N_TRUE;
	
	return N_FALSE;
}

// 绘制文件选择控件
nbool NBK_paintBrowse(
    void* pfd, const NFontId fontId, const NRect* rect,
    const wchr* text, int len, NECtrlState state, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    CNbkGdi* gdi = (CNbkGdi*)p->gdi;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;
    
    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());

    TPtrC label;
    if (text)
        label.Set(text, len);
    else
        label.Set(KNullDesC);
    MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;
    
    const CFont* font = gdi->GetZoomFont(fontId);

    TRect focusR;
    k->GetFocusRect(k->GetFocus(), focusR);

    NRect viewPort;
    k->GetViewPort(&viewPort);
    TPoint tmp(-viewPort.l,-viewPort.t);
    focusR.Move(tmp);
    
    if (focusR.Intersects(r)) {
        if (MNbkCtrlPaintInterface::EPressed == k->CurNodeState())
            st = MNbkCtrlPaintInterface::EPressed;
    }
    
    TBool ret = (cpi && cpi->OnBrowsePaint(k->GetScreenGc(), font, r, label, st, c));
    return ret; 
}

// 绘制折叠块背景
nbool NBK_paintFoldBackground(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    CNbkGdi* gdi = (CNbkGdi*)p->gdi;
    MNbkCtrlPaintInterface *cpi = (MNbkCtrlPaintInterface*)p->ctlPainter;

    TRect r(rect->l, rect->t, rect->r, rect->b);
    TRect c(clip->l, clip->t, clip->r, clip->b);
    r.Move(gdi->GetDrawOffset());
    c.Move(gdi->GetDrawOffset());
    MNbkCtrlPaintInterface::TCtrlState st = (MNbkCtrlPaintInterface::TCtrlState)state;

    if (cpi && cpi->OnFoldCtlPaint(k->GetScreenGc(), r, st, c))
        return N_TRUE;
    
    return N_FALSE;
}

// 请求框 架文件选择对话框
nbool NBK_browseFile(void* pfd, const char* oldFile, char** newFile)
{
    CNBrKernel* k = (CNBrKernel*)((NBK_PlatformData*)pfd)->nbk;
    
    wchr* old = NULL;
    int len = 0;
    
    if (oldFile)
        old = uni_utf8_to_utf16_str((uint8*)oldFile, -1, &len);
    
    TPtrC oldPtr(old, len);
    HBufC* newH = NULL;
    
    // 调用框架选择文件
    nbool ret = k->SelectFile(oldPtr, newH);
    if (old)
        NBK_free(old);
    
    if (ret && newH) {
        TPtrC p = newH->Des();
        *newFile = uni_utf16_to_utf8_str((wchr*)p.Ptr(), p.Length(), NULL);
    }
    
    if (newH)
        delete newH;
    
    return ret;
}

// 提示对话框
nbool NBK_dlgAlert(void* pfd, const wchr* text, int len)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    MNbkDialogInterface* dlg = (MNbkDialogInterface*)p->dlgImpl;

    nbool rt = N_FALSE;
    if (dlg) {
        TPtrC msg(text, len);
        k->DenyEvent(ETrue);
        dlg->OnAlert(msg);
        k->DenyEvent(EFalse);
        rt = N_TRUE;
    }
    
    return rt;
}

// 验证对话框
nbool NBK_dlgConfirm(void* pfd, const wchr* text, int len, int* ret)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    MNbkDialogInterface* dlg = (MNbkDialogInterface*)p->dlgImpl;

    nbool rt = N_FALSE;
    if (dlg) {
        TPtrC msg(text, len);
        k->DenyEvent(ETrue);
        TBool choose = dlg->OnConfirm(msg);
        k->DenyEvent(EFalse);
        *ret = (choose) ? 1 : 0;
        rt = N_TRUE;
    }
    
    return rt;
}

// 输入对话框
nbool NBK_dlgPrompt(void* pfd, const wchr* text, int len, int* ret, char** input)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* k = (CNBrKernel*)p->nbk;
    MNbkDialogInterface* dlg = (MNbkDialogInterface*)p->dlgImpl;

    nbool rt = N_FALSE;
    if (dlg) {
        TPtrC msg(text, len);
        HBufC* newH = NULL;
        k->DenyEvent(ETrue);
        TBool choose = dlg->OnPrompt(msg, newH);
        k->DenyEvent(EFalse);
        *ret = (choose) ? 1 : 0;
        if (choose && newH) {
            TPtrC p = newH->Des();
            *input = uni_utf16_to_utf8_str((wchr*)p.Ptr(), p.Length(), NULL);
        }
        rt = N_TRUE;
        
        if (newH)
            delete newH;
    }
    
    return rt;
}

// 当单行控件进入编辑模式时，调用该接口，该接口将被多次调用，显示位置可能不同
nbool NBK_editInput(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len, int maxlen, nbool password)
{
#if ENABLE_FEP
    return EFalse;
#else
    CFEPControl* control = CResourceManager::GetPtr()->GetFEPControl();
    if (control->IsEdit()) {

        NBK_PlatformData* p = (NBK_PlatformData*) pfd;
        CNBrKernel* k = (CNBrKernel*) p->nbk;
        CNbkGdi* gdi = (CNbkGdi*) p->gdi;
           
        TRect r(rect->l, rect->t, rect->r, rect->b);
        CFont* useFont = (CFont*) (gdi->GetFont(fontId));

        if (control->iSrcEditRect != r) {
            control->iSrcEditRect = r;
            r.Move(gdi->GetDrawOffset());

            NRect viewPort;
            k->GetViewPort(&viewPort);
            TInt w = rect_getWidth(&viewPort);
            TInt h = rect_getHeight(&viewPort);
            
            if (r.iTl.iX < 0)
                r.SetRect(0, r.iTl.iY, r.iBr.iX, r.iBr.iY);
            if (r.iBr.iX > w)
                r.SetRect(r.iTl.iX, r.iTl.iY, w, r.iBr.iY);

            if (r.iTl.iY < 0)
                r.SetRect(r.iTl.iX, 0, r.iBr.iX, r.iBr.iY);
            if (r.iBr.iY > h)
                r.SetRect(r.iTl.iX, r.iTl.iY, r.iBr.iX, h);

            if (r.Width() == w)
                r.Shrink(DEFAULT_FEP_HORI_SPACING / 2, 0);
            if (r.Height() == h)
                r.Shrink(0, DEFAULT_FEP_VERT_SPACING / 2);
            
            TInt YOffet;
            if (useFont) {
                TInt pixel = gdi->GetZoomFontPixel(useFont->FontMaxHeight());
                //pixel -= 3;
                YOffet = (r.Height() - pixel - 4) / 2;
            }

            NPoint offset;
            k->GetNBKOffset(&offset);
            r.Move(offset.x, offset.y);
            TSize shrikSize = k->GetFepShrinSize();
            if (YOffet > 0 && YOffet != shrikSize.iHeight)
                shrikSize.SetSize(shrikSize.iWidth, YOffet);
            r.Shrink(shrikSize);       
            
            if (password) {
                control->SetType(CFEPControl::CONTROL_PASSWORD);
            }
            else {
                control->SetType(CFEPControl::CONTROL_EDITBOX);
            }
            control->SetRect(r);
        }     
        
        if (!control->IsActive()) {
            TBool ControlOwnFont = EFalse;
            if (useFont) {
                TFontSpec spec = useFont->FontSpecInTwips();
                TInt pixel = gdi->GetZoomFontPixel(useFont->FontMaxHeight());
                TBool bold = (TBool) spec.iFontStyle.StrokeWeight();
                TBool italic = (TBool) spec.iFontStyle.Posture();
//                useFont = gdi->GetCacheFont(pixel, bold, italic);
//                if (!useFont) {
                    ControlOwnFont = ETrue;
                    useFont = gdi->GetFontMgr()->CreateFont(pixel, bold, italic);
//                }
            }
            if (k && k->IsNightTheme()) {
                NNightTheme* theme = (NNightTheme*) k->GetNightTheme();
                TRgb color((TInt) (theme->text.r), (TInt) (theme->text.g), (TInt) (theme->text.b),
                    (TInt) (theme->text.a));
                TRgb bgColor((TInt) (theme->background.r), (TInt) (theme->background.g),
                    (TInt) (theme->background.b), (TInt) (theme->background.a));
                control->SetFontColor(color);
                control->SetBgColor(bgColor);
            }
            control->SetMaxLength(maxlen);
            control->SetFont(useFont, ControlOwnFont);
            control->SetInpuMode(CFEPControl::EText);
            control->Active();

            //_LIT(tmp,"Baidu@\x8F93\x5165\x5185\x5BB9");
            TPtrC16 initText(KNullDesC);
            if (len)
                initText.Set(text, len);
            len = initText.Length();
            control->SetText(initText);
            control->SetCursor(len);
            NBK_fep_updateCursor(pfd);
            //control->ReportAknEdStateEventL(500);           
        }
        control->DrawNow();
        return N_TRUE;
    }
    return N_FALSE;
#endif
}

// 当多行编辑控件进入编辑模式时，调用该接口
nbool NBK_editTextarea(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len, int maxlen)
{
#if ENABLE_FEP
    return EFalse;
#else
    CFEPControl* control = CResourceManager::GetPtr()->GetFEPControl();
    if (control->IsEdit()) {

        TRect r(rect->l, rect->t, rect->r, rect->b);
        NBK_PlatformData* p = (NBK_PlatformData*) pfd;
        CNBrKernel* nbk = (CNBrKernel*) p->nbk;
        CNbkGdi* gdi = (CNbkGdi*) p->gdi;
        CFont* useFont = (CFont*) (gdi->GetFont(fontId));

        if (control->iSrcEditRect != r) {
            control->iSrcEditRect = r;
            
            r.Move(gdi->GetDrawOffset());
            NRect viewPort;
            nbk->GetViewPort(&viewPort);
            TInt w = rect_getWidth(&viewPort);
            TInt h = rect_getHeight(&viewPort);

            if (r.iTl.iX < 0)
                r.SetRect(0, r.iTl.iY, r.iBr.iX, r.iBr.iY);
            if (r.iBr.iX > w)
                r.SetRect(r.iTl.iX, r.iTl.iY, w, r.iBr.iY);

            if (r.iTl.iY < 0)
                r.SetRect(r.iTl.iX, 0, r.iBr.iX, r.iBr.iY);
            if (r.iBr.iY > h)
                r.SetRect(r.iTl.iX, r.iTl.iY, r.iBr.iX, h);

            if (r.Width() == w)
                r.Shrink(DEFAULT_FEP_HORI_SPACING / 2, 0);
            if (r.Height() == h)
                r.Shrink(0, DEFAULT_FEP_VERT_SPACING / 2);

            NPoint offset;
            nbk->GetNBKOffset(&offset);
            r.Move(offset.x, offset.y);
            TSize shrikSize = nbk->GetFepShrinSize();
            r.Shrink(shrikSize);
            control->SetType(CFEPControl::CONTROL_MULTEDITBOX);
            control->SetRect(r);
        }
        
        if (!control->IsActive()) {

            TBool ControlOwnFont = EFalse;
            if (useFont) {
                TFontSpec spec = useFont->FontSpecInTwips();

                TInt pixel = gdi->GetZoomFontPixel(useFont->FontMaxHeight());
                TBool bold = (TBool) spec.iFontStyle.StrokeWeight();
                TBool italic = (TBool) spec.iFontStyle.Posture();
//                useFont = gdi->GetCacheFont(pixel, bold, italic);
//                if (!useFont) {
                    ControlOwnFont = ETrue;
                    useFont = gdi->GetFontMgr()->CreateFont(pixel, bold, italic);
//                }
            }
            if (nbk && nbk->IsNightTheme()) {
                NNightTheme* theme = (NNightTheme*) nbk->GetNightTheme();
                TRgb color((TInt) (theme->text.r), (TInt) (theme->text.g), (TInt) (theme->text.b),
                    (TInt) (theme->text.a));
                TRgb bgColor((TInt) (theme->background.r), (TInt) (theme->background.g),
                    (TInt) (theme->background.b), (TInt) (theme->background.a));
                control->SetFontColor(color);
                control->SetBgColor(bgColor);
            }
            control->SetInpuMode(CFEPControl::EText);
            control->SetFont(useFont, ControlOwnFont);
            control->SetMaxLength(maxlen);
            control->Active();
            //_LIT(tmp,"Baidu@\x8F93\x5165\x5185\x5BB9");
            TPtrC16 initText(KNullDesC);
            if (len)
                initText.Set(text, len);
            len = initText.Length();
            control->SetText(initText);
            control->SetCursor(len);
            NBK_fep_updateCursor(pfd);
            //control->ReportAknEdStateEventL(500);
        }
        
        control->DrawNow();        
        return N_TRUE;
    }
    return N_FALSE;
#endif
}

// 绘制内置图片
void NBK_drawStockImage(void* pfd, const char* res, const NRect* rect, NECtrlState state, const NRect* clip)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
}
