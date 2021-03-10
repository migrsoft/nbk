/*
 ============================================================================
 Name		: testappAppView.h
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Declares view class for application.
 ============================================================================
 */

#ifndef __TESTAPPAPPVIEW_h__
#define __TESTAPPAPPVIEW_h__

#include <NBrKernel.h>
#include <coecntrl.h>
#include <fbs.h>

class CtestappAppView : public CCoeControl,
                        public MNbrKernelInterface,
                        public MNbkCtrlPaintInterface,
                        public MNbkDialogInterface
{
public:
    static CtestappAppView* NewL(const TRect& aRect);
    static CtestappAppView* NewLC(const TRect& aRect);
    virtual ~CtestappAppView();

protected:
    virtual TCoeInputCapabilities InputCapabilities() const;
    virtual void FocusChanged(TDrawNow aDrawNow);

public:
    void Draw(const TRect& aRect) const;
    virtual void SizeChanged();
    
    virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);
    TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
    
    void SetDirectWorkMode();
    void SwitchNbkWorkMode();
    void SwitchNbkWorkModeCute();
    void SwitchNbkWorkModeUck();
    void EnableImage();
    void EnableIncMode();
    void LoadData(const TDesC8& url, HBufC8* aData);
    void LoadUrl(const TDesC8& url);
    void Stop(void);
    void Refresh(void);
    void Back();
    void Forward();
    void ClearFileCache();
    void SwitchNightScheme();
    void MonkeyTest();
    void ChangeLayoutMode();
    void RememberUri();
    void CopyTest();
    
public:
    // from MNbrKernelInterface
    virtual void NbkRedraw();
    virtual void NbkRequestAP();
    virtual void OnNewDocument() {}
    virtual void OnHistoryDocument() {}
    virtual void OnWaiting() {}
    virtual void OnNewDocumentBegin() {}
    virtual void OnGotResponse() {}
    virtual void OnReceivedData(int aReceived, int aTotal) {}
    virtual void OnReceivedImage(int aCurrent, int aTotal) {}
    virtual void OnViewUpdate(TBool aSizeChange);
    virtual void OnLayoutFinish(int aX, int aY, float aZoom);
    virtual void OnDownload(const TDesC8& aUrl, const TDesC16& aFileName, const TDesC8& aCookie, TInt aSize);
    virtual void OnEnterMainBody() {}
    virtual void OnQuitMainBody(TBool aRefresh) {}
    virtual TBool OnSelectFile(TDesC& aOldFile, HBufC*& aSelectedFile);
    virtual void OnGetDocViewPort(TRect& aRect);
    // 获取内核显示区域在屏幕中的偏移量
    virtual void OnGetNBKOffset(TPoint& aPoint);
    virtual void OnEditorStateChange(TBool& aEdit){};
    
public:
    // from MNbkCtrlPaintInterface
    virtual TBool OnInputTextPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip);
    virtual TBool OnCheckBoxPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, TBool aChecked, const TRect& aClip);
    virtual TBool OnRadioPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, TBool aChecked, const TRect& aClip);
    virtual TBool OnButtonPaint(CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip);
    virtual TBool OnSelectNormalPaint(CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip);
    virtual TBool OnSelectExpandPaint(const RArray<TPtrC>& aItems, TInt& aSel);
    virtual TBool OnTextAreaPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip);
    virtual TBool OnBrowsePaint(CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip);
    virtual TBool OnFoldCtlPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip);

public:
    // from MNbkDialogInterface
    virtual void OnAlert(const TDesC& aMessage);
    virtual TBool OnConfirm(const TDesC& aMessage);
    virtual TBool OnPrompt(const TDesC& aMessage, HBufC*& aInputted);
    
private:
    void ConstructL(const TRect& aRect);
    CtestappAppView();
    
    TKeyResponse ProcessKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
    
    void ChangeLayoutMode(TBool aSelfAdaption);
    
    CNBrKernel* iNbk;
    CFbsBitmap* iNbkScreen;
    TBool iNightView;
    TBool iSelfAdaption;
    
    TPoint iViewPos;
    
    TBuf<KMaxTypefaceNameLength> iTypefaceName;
    TInt    ifontID;
    TBool iMonkeyRun;
    TRect iNBKRect;
    //CImageImpl* iTestImage;
    
    TBool iCopyMode;
};
#endif // __TESTAPPAPPVIEW_h__
