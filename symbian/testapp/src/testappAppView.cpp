/*
 ============================================================================
 Name		: testappAppView.cpp
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Application view implementation
 ============================================================================
 */

// INCLUDE FILES
#include <coemain.h>
#include <math.h>
#include "testappAppView.h"
#include "testappAppUi.h"
#include "netTypeDef.h"

_LIT(KCMCCCmwap, "cmwap");
_LIT(KUniCmwap, "uniwap");

CtestappAppView* CtestappAppView::NewL(const TRect& aRect)
{
    CtestappAppView* self = CtestappAppView::NewLC(aRect);
    CleanupStack::Pop(self);
    return self;
}

CtestappAppView* CtestappAppView::NewLC(const TRect& aRect)
{
    CtestappAppView* self = new (ELeave) CtestappAppView;
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    return self;
}

void CtestappAppView::ConstructL(const TRect& aRect)
{
    iNbk = CNBrKernel::NewL(this);
    iNbk->SetAutoMode(ETrue);
//    iNbk->SetFepYOffset(20);
//    iNbk->SetFontAntiAliasing(ETrue);
//    iNbk->SetControlPainter(this);
    iNbk->SetDialogInterface(this);
    ChangeLayoutMode(iSelfAdaption);
    iNBKRect = aRect;
    CreateWindowL();
    SetRect(aRect);

    iNbk->FepInit(this);
    
    Window().SetPointerGrab(ETrue);
    Window().PointerFilter(EPointerFilterEnterExit | EPointerFilterDrag | EPointerFilterMove | EPointerGenerateSimulatedMove, 0);
    Window().SetPointerCapture(RWindowBase::TCaptureFlagDragDrop);
    
    iViewPos.SetXY(0, 0);

    TTime now;
    now.HomeTime();
    TDateTime dt = now.DateTime();
    int hour = dt.Hour();
    iNightView = (hour > 6 && hour < 22) ? EFalse : ETrue;

    ActivateL();
}

CtestappAppView::CtestappAppView()
    : iMonkeyRun(EFalse)
{
#ifdef __WINSCW__
    iSelfAdaption = EFalse;
#else
    iSelfAdaption = ETrue;
#endif
}

CtestappAppView::~CtestappAppView()
{
    delete iNbkScreen;
    delete iNbk;
}

void CtestappAppView::Draw(const TRect& /*aRect*/) const
{    
    CWindowGc& gc = SystemGc();

    gc.BitBlt(iViewPos, iNbkScreen);
    
    if (iNightView) {
        gc.SetBrushColor(TRgb(0,0,0,192));
        gc.Clear();
    }
}

void CtestappAppView::SizeChanged()
{
    TRect rect = Rect();

    bool createBitmap = false;
    if (iNbkScreen == NULL)
        createBitmap = true;
    else {
        TSize sz = iNbkScreen->SizeInPixels();
        if (sz.iWidth != rect.Width())
            createBitmap = true;
    }
    
    if (createBitmap) {
        if (iNbkScreen)
            delete iNbkScreen;
        iNbkScreen = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(iNbkScreen);
        TSize sz = rect.Size();
        User::LeaveIfError(iNbkScreen->Create(sz, EColor64K));
        CleanupStack::Pop();
        
        iNbk->SetScreen(iNbkScreen);
    }

    DrawNow();
}

void CtestappAppView::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
    iNbk->HandlePointerEventL(aPointerEvent);
}

void CtestappAppView::NbkRedraw()
{
    DrawNow();
}

void CtestappAppView::NbkRequestAP()
{
    CtestappAppUi* ui = (CtestappAppUi*)CCoeEnv::Static()->AppUi();
    TEApList ap;
    TBool ret = ui->GetIapL(ap);
    TBool isWap = EFalse;
    if (ap.iApn == KCMCCCmwap || ap.iApn == KUniCmwap) {
        isWap = true;
    }
    if (ret)
        iNbk->SetIap(ap.iId, isWap);
}

void CtestappAppView::OnLayoutFinish(int aX, int aY, float aZoom)
{
/*
    TPtrC8 mainUrl = iNbk->GetUrl();
    TPtrC docTitle = iNbk->GetTitle();
    void* focus = iNbk->GetFocus(TRect(0, 0, 240, 320));
    HBufC8* focusUrl = iNbk->GetFocusUrl(focus);
    if (focusUrl)
        delete focusUrl;
*/
    
//    int w = iNbk->GetWidth();
//    int h = iNbk->GetHeight();
//    TSize sz = iNbkScreen->SizeInPixels();
//    
//    iViewPos.SetXY(0, 0);
//    
//    TRect draw(0, 0, w, Min(h, sz.iHeight));
//    iNbk->ToDocCoords(draw);
//    iNbk->DrawPage(draw);
//    
//    DrawNow();

    // 测试局部绘制
//    iNbk->DrawPage(TRect(10,50,100,200), TPoint(50,50));
//    DrawNow();
}

void CtestappAppView::LoadData(const TDesC8& url, HBufC8* aData)
{
    iNbk->LoadData(url, aData);
}

void CtestappAppView::LoadUrl(const TDesC8& url)
{
    iNbk->LoadUrl(url);
}

TKeyResponse CtestappAppView::OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
//    return ProcessKeyEventL(aKeyEvent, aType);
    
    if (iNbk->OfferKeyEventL(aKeyEvent, aType) == EKeyWasNotConsumed) {
        
#if 1
        switch (aKeyEvent.iCode) {
        case 'o':
        {
            float fa = iNbk->GetCurrentZoomFactor();
            fa -= 0.1;
            iNbk->ZoomPage(fa);
            return EKeyWasConsumed;
        }
        case 'p':
        {
            float fa = iNbk->GetCurrentZoomFactor();
            if (fabs(iNbk->GetMinZoomFactor() - fa) < 0.001) {
                fa += 0.1;
                TInt tmp = fa * 100;
                fa = (float) ((tmp % 10) / 10);
            }
            else
                fa += 0.1;
            iNbk->ZoomPage(fa);
            return EKeyWasConsumed;
        }
        case 'z':
        {
            ((CAknAppUi*)CCoeEnv::Static()->AppUi())->Exit();
            return EKeyWasConsumed;
        }
        case 'b':
            iNbk->Back();
            return EKeyWasConsumed;
        case 'f':
            iNbk->Forward();
            return EKeyWasConsumed;
        }
#endif
        return CCoeControl::OfferKeyEventL(aKeyEvent, aType);
    }
    else
        return EKeyWasConsumed;
}

TKeyResponse CtestappAppView::ProcessKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType)
{
    TKeyResponse resp = EKeyWasConsumed;
    
    if (aType == EEventKey) {
        switch (aKeyEvent.iScanCode) {
        
        case EStdKeyLeftArrow:
            break;
            
        case EStdKeyRightArrow:
            break;
            
        case EStdKeyUpArrow:
            iViewPos.iY += 10;
            DrawNow();
            break;
            
        case EStdKeyDownArrow:
            iViewPos.iY -= 10;
            DrawNow();
            break;
            
        case EStdKeyDevice3:
            break;
            
        default:
            resp = EKeyWasNotConsumed;
        }
    }
    
    return resp;
}

void CtestappAppView::SetDirectWorkMode()
{
    iNbk->SetWorkMode(CNBrKernel::EDirect);
}

void CtestappAppView::SwitchNbkWorkMode()
{
    CNBrKernel::EWorkMode mode = iNbk->GetWorkMode();
    
    if (mode != CNBrKernel::EFffull)
        mode = CNBrKernel::EFffull;
    else
        mode = CNBrKernel::EDirect;
    
    if (mode != CNBrKernel::EUnknown) {
        iNbk->SetWorkMode(mode);
        iNbk->SetWorkModeForTC(mode);
    }
    
    _LIT(KMess,     "Switch Mode to:");
    _LIT(KModeFull, "ff-full");
    _LIT(KModeDir,  "direct ");
    CEikonEnv::Static()->InfoWinL(KMess, \
        (iNbk->GetWorkMode() == CNBrKernel::EDirect) ? KModeDir : KModeFull);
}

void CtestappAppView::SwitchNbkWorkModeCute()
{
    CNBrKernel::EWorkMode mode = iNbk->GetWorkMode();
    
    if (mode != CNBrKernel::EFfNarrow)
        mode = CNBrKernel::EFfNarrow;
    else
        mode = CNBrKernel::EDirect;
    
    if (mode != CNBrKernel::EUnknown) {
        iNbk->SetWorkMode(mode);
        iNbk->SetWorkModeForTC(mode);
    }
    
    _LIT(KMess,     "Switch Mode to:");
    _LIT(KModeCute, "ff-cute");
    _LIT(KModeDir,  "direct ");
    CEikonEnv::Static()->InfoWinL(KMess, \
        (iNbk->GetWorkMode() == CNBrKernel::EDirect) ? KModeDir : KModeCute);
}

void CtestappAppView::SwitchNbkWorkModeUck()
{
    CNBrKernel::EWorkMode mode = iNbk->GetWorkMode();
    
    if (mode != CNBrKernel::EUck)
        mode = CNBrKernel::EUck;
    else
        mode = CNBrKernel::EDirect;
    
    if (mode != CNBrKernel::EUnknown) {
        iNbk->SetWorkMode(mode);
        iNbk->SetWorkModeForTC(mode);
    }
    
    _LIT(KMess,     "Switch Mode to:");
    _LIT(KModeUck,  "uck    ");
    _LIT(KModeDir,  "direct ");
    CEikonEnv::Static()->InfoWinL(KMess, \
        (iNbk->GetWorkMode() == CNBrKernel::EDirect) ? KModeDir : KModeUck);
}

TCoeInputCapabilities CtestappAppView::InputCapabilities() const
{
    return iNbk->InputCapabilities();
}

void CtestappAppView::OnDownload(const TDesC8& aUrl, const TDesC16& aFileName, const TDesC8& aCookie, TInt aSize)
{
    TBuf<64> m1;
    TBuf<32> m2;
    
    m1 = _L("File: ");
    if (aFileName.Length())
        m1 += aFileName;
    else
        m1 += _L("unknown");
    
    m2.Format(_L("Size: %d"), aSize);
    
    CEikonEnv::Static()->InfoWinL(m1, m2);
}

void CtestappAppView::Stop(void)
{
    iNbk->Stop();
}

void CtestappAppView::Refresh(void)
{
    iNbk->Refresh();
}

void CtestappAppView::EnableImage()
{
    TBool enable = iNbk->AllowImage();
    enable = !enable;
    iNbk->SetAllowImage(enable);
    
    _LIT(KMess, "Image:");
    _LIT(KModeOn, "On ");
    _LIT(KModeOff, "Off");
    CEikonEnv::Static()->InfoWinL(KMess, (enable) ? KModeOn : KModeOff);
}

void CtestappAppView::EnableIncMode()
{
//    TBool enable = iNbk->IncreasedMode();
//    enable = !enable;
//    iNbk->SetIncreasedMode(enable);
//    
//    _LIT(KMess, "Increased Mode:");
//    _LIT(KModeOn, "On ");
//    _LIT(KModeOff, "Off");
//    CEikonEnv::Static()->InfoWinL(KMess, (enable) ? KModeOn : KModeOff);
}

void CtestappAppView::Back()
{
    iNbk->Back();
}

void CtestappAppView::Forward()
{
    iNbk->Forward();
}

TBool CtestappAppView::OnInputTextPaint(
    CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip)
{
    aGc.SetBrushColor(KRgbWhite);
    aGc.Clear(aRect);

    if (aState == EHighlight)
        aGc.SetPenColor(KRgbBlue);
    else if (aState == EInEdit)
        aGc.SetPenColor(KRgbYellow);
    else
        aGc.SetPenColor(KRgbWhite);
    aGc.DrawRect(aRect);
    
    return ETrue;
}

TBool CtestappAppView::OnCheckBoxPaint(
    CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, TBool aChecked, const TRect& aClip)
{
    aGc.SetBrushColor(KRgbMagenta);
    aGc.Clear(aRect);

    if (aState == EHighlight)
        aGc.SetPenColor(KRgbBlue);
    else
        aGc.SetPenColor(KRgbWhite);
    aGc.DrawRect(aRect);

    return ETrue;
}

TBool CtestappAppView::OnRadioPaint(
    CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, TBool aChecked, const TRect& aClip)
{
    aGc.SetBrushColor(KRgbMagenta);
    aGc.Clear(aRect);

    if (aState == EHighlight)
        aGc.SetPenColor(KRgbBlue);
    else
        aGc.SetPenColor(KRgbWhite);
    aGc.DrawRect(aRect);

    return ETrue;
}

TBool CtestappAppView::OnButtonPaint(
    CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip)
{
    aGc.SetBrushColor(KRgbWhite);
    aGc.Clear(aRect);

    if (aState == EHighlight)
        aGc.SetPenColor(KRgbBlue);
    else
        aGc.SetPenColor(KRgbBlack);
    aGc.DrawRect(aRect);

    aGc.UseFont(aFont);
    TPoint pt = aRect.iTl;
    pt.iX += 1;
    pt.iY += aFont->AscentInPixels() + 1;
    aGc.DrawText(aText, pt);
    aGc.DiscardFont();
    
    return ETrue;
}

TBool CtestappAppView::OnSelectNormalPaint(
    CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip)
{
    aGc.SetBrushColor(KRgbWhite);
    aGc.Clear(aRect);

    if (aState == EHighlight)
        aGc.SetPenColor(KRgbBlue);
    else
        aGc.SetPenColor(KRgbBlack);
    aGc.DrawRect(aRect);
    
    aGc.UseFont(aFont);
    TPoint pt = aRect.iTl;
    pt.iX += 1;
    pt.iY += aFont->AscentInPixels() + 1;
    aGc.DrawText(aText, pt);
    aGc.DiscardFont();

    return ETrue;
}

TBool CtestappAppView::OnSelectExpandPaint(const RArray<TPtrC>& aItems, TInt& aSel)
{
    int a = aSel;
    int b = a + 1;
    
    if (b == aItems.Count())
        b = 0;
    
    CEikonEnv::Static()->InfoWinL(aItems[a], aItems[b]);
    
    aSel++;
    if (aSel == aItems.Count())
        aSel = 0;
    
    return ETrue;
}

TBool CtestappAppView::OnTextAreaPaint(
    CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip)
{
    aGc.SetBrushColor(KRgbWhite);
    aGc.Clear(aRect);

    if (aState == EHighlight)
        aGc.SetPenColor(KRgbBlue);
    else if (aState == EInEdit)
        aGc.SetPenColor(KRgbYellow);
    else
        aGc.SetPenColor(KRgbWhite);
    aGc.DrawRect(aRect);
    
    return ETrue;
}

void CtestappAppView::ClearFileCache()
{
//    iNbk->ClearFileCache();
//    iNbk->ClearCookies();
//    iNbk->ClearFormData();
}

void CtestappAppView::OnViewUpdate(TBool aSizeChange)
{
//    if (aSizeChange) {
//        _LIT8(KFmt, "w=%d\th=%d\tz=%1.2f");
//        int w = iNbk->GetWidth();
//        int h = iNbk->GetHeight();
//        float z = iNbk->GetCurrentZoomFactor();
//        TBuf8<32> out;
//        out.Format(KFmt, w, h, z);
//        iNbk->Dump((char*)out.Ptr(), out.Length());
//    }
}

TBool CtestappAppView::OnSelectFile(TDesC& aOldFile, HBufC*& aSelectedFile)
{
    _LIT(KTestFile, "c:\\data\\tt.png");
    TPtrC p(KTestFile);
    aSelectedFile = p.AllocL();
    return ETrue;
}

void CtestappAppView::OnGetDocViewPort(TRect& aRect)
{
    
}

void CtestappAppView::OnGetNBKOffset(TPoint& aPoint)
{
    aPoint = iNBKRect.iTl;
}

TBool CtestappAppView::OnBrowsePaint(
    CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip)
{
    return EFalse;
}

TBool CtestappAppView::OnFoldCtlPaint(
    CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip)
{
    aGc.SetBrushColor((aState == ENormal) ? KRgbBlue : KRgbRed);
    aGc.Clear(aRect);
    return ETrue;
}

void CtestappAppView::OnAlert(const TDesC& aMessage)
{
    CEikonEnv::Static()->InfoWinL(aMessage, KNullDesC);
}

TBool CtestappAppView::OnConfirm(const TDesC& aMessage)
{
    return CEikonEnv::Static()->QueryWinL(aMessage, KNullDesC);
}

TBool CtestappAppView::OnPrompt(const TDesC& aMessage, HBufC*& aInputted)
{
    _LIT(KTest, "60");
    
    TBool ret = CEikonEnv::Static()->QueryWinL(aMessage, KNullDesC);
    TPtrC p(KTest);
    aInputted = p.Alloc();
    
    return ret;
}

void CtestappAppView::SwitchNightScheme()
{
    iNightView = !iNightView;
    DrawNow();
}

void CtestappAppView::MonkeyTest()
{
#ifdef __NBK_SELF_TESTING__
    _LIT8(TestUrl,"http://i.ifeng.com");
    if (!iMonkeyRun) {
        iMonkeyRun = ETrue;
        TUid uid = TUid::Uid(0xEA69F97C);
        iNbk->StartMonkey(TestUrl, CNBrKernel::EDirect, CNBrKernel::eHitPageLinks, 0,7200,uid);
    }
    else {
        iMonkeyRun = EFalse;
        iNbk->StopMonkey();
    }
#endif
}

void CtestappAppView::ChangeLayoutMode(TBool aSelfAdaption)
{
    if (aSelfAdaption) {
        iNbk->SetSelfAdaptionLayout(ETrue);
        iNbk->SetFontSize(22, CNBrKernel::EFontMiddle);
    }
    else {
        iNbk->SetSelfAdaptionLayout(EFalse);
        iNbk->SetFontSize(0, CNBrKernel::EFontMiddle);
    }
}

void CtestappAppView::ChangeLayoutMode()
{
    iSelfAdaption = !iSelfAdaption;
    
    ChangeLayoutMode(iSelfAdaption);
}

void CtestappAppView::RememberUri()
{
    const TPtrC8 uri = iNbk->GetUrl();
    iNbk->Dump((char*)uri.Ptr(), uri.Length());
}

void CtestappAppView::FocusChanged(TDrawNow aDrawNow)
{
//    if (!IsFocused())
//        iNbk->SetFocus(NULL);
}

void CtestappAppView::CopyTest()
{
    if (iCopyMode) {
        iCopyMode = EFalse;
        iNbk->CopyModeEnd();
    }
    else {
        iCopyMode = ETrue;
        iNbk->CopyModeBegin();
    }
}
