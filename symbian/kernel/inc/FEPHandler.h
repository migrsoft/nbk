#if !defined(_PCTI_FEPHandler_h)
#define _PCTI_FEPHandler_h

#include "COECNTRL.h"
#include "FEPBASE.h"
#include "e32cmn.h"
#include <e32base.h>
#include <aknedstsobs.h> 
#include <eikon.hrh>
#include "../../../stdc/inc/config.h"
#include "../../../stdc/inc/nbk_gdi.h"

#include <eikedwin.h>
#include <eikseced.h>
#include <eikedwob.h>

#define ENABLE_FEP  1
#define DEFAULT_FEP_HORI_SPACING    8
#define DEFAULT_FEP_VERT_SPACING    8

class CProbe;
class CAknEdwinState;
class CNBrKernel;

//自定义密码框，禁用CCPU功能
class CSecretEditor: public CEikEdwin
{
public:
    // Constructors and destructor 
    CSecretEditor();
    virtual ~CSecretEditor();
    void ConstructL(TInt aCount);
    void Draw(const TRect& aRect) const;
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void GetPassword(TDes& aDes) const;
    void SetPassword(const TDesC& aTxt);
    void HideControlText(void);
    void SetFont(CFont* aFont){ iFont = aFont; }
    void UpdateCursor(void);
    
    void StartCursorL(TInt aDelay);
    void StopCursor(void);
    
private:
    void UpdateDisplayPassword(void);
    void OnInsertPassword(const TDesC& aNewChars,TInt aCurosrPos);
    void OnDeletePassword(TInt curPos,TBool aBackspace);
    void ConvertPassword(void);
    // Functions from base classes 
    TInt iCount;
    HBufC* iPassword;
    HBufC* iDisplayPassword;
    //TInt iCursorPos;
    //HBufC* iTextOnEdit;
    CFont* iFont;
    
    TBool iShowCursor;
    CPeriodic* iCursorDelay;
    static TInt OnUpdateCurosr(TAny* aSelf);
    void DoUpdateCursor(void);
};

class CSecretEditor1: public CEikSecretEditor
{
public:
    // Constructors and destructor 
    CSecretEditor1();
    virtual ~CSecretEditor1();
    void ConstructL(void);
    void SetFont(CFont* aFont);

private:
    virtual void Draw(const TRect& aRect) const;
    CFont* iCustomFont;

};

class MFEPControlObserver
{
public:
    virtual void HandleSysControlContentChanged(void) = 0;
    virtual void HandleSysControlStateChanged(void) = 0;//输入状态切换
};

class CFEPControl: public CCoeControl, public MEikEdwinObserver
{
public:
    enum
    {
        MAX_PASSWORD = 128, MAX_TEXTEDITOR = 256, MAX_MULTTEXTEDITOR = 4096,
    };

    typedef enum _ControlType
    {
        CONTROL_NONE, CONTROL_PASSWORD, CONTROL_EDITBOX, CONTROL_MULTEDITBOX,
    }ControlType;

    enum TInputMode
    {
        EText, ENumber, EUrl
    };

    ~CFEPControl();

    static CFEPControl* NewL(void);
    static CFEPControl* NewLC(void);

protected:
    CFEPControl();
    void ConstructL();

public:
    void SetObserver(MFEPControlObserver* aObserver);
    void SetEditState(CNBrKernel* aNBK, TBool aEdit);
    void SetSelection(CNBrKernel* aNBK);
    
    TBool IsEdit(void) { return iEditState; }
    TBool IsActive();
    void Active(void);
    void DeActive();
    void SetControlFocus(TBool aFocus);
    void SetType(ControlType aType)
    {
        iControlType = aType;
    }
    void PasteTextL(CNBrKernel* aNBK,const TDesC& aText);

    //属性
    void SetRect(const TRect& aRect);
    void SetBgColor(TRgb aColor);
    void SetFont(CFont* aFont,TBool aOwn);
    CFont*& GetFont();
    void SetFontColor(TRgb aColor);
    void SetText(const TDesC& aText,TBool aInsert = EFalse);
    void GetText(TDes& aText);
    TInt GetTextLength();
    TInt GetCursor();
    void SetCursor(TInt aPos);
    void SetSelection(TInt aStartPos, TInt aEndPos);
    void SelectAllL();

    void SetMaxLength(TInt aMaxLen);
    
    TInt GetFontHeight(ControlType aType, TFontSpec aFontSpec);
    TInt GetLineSpace(ControlType aType);

    void SetInpuMode(TInputMode aMode);
    void HandleTextChangedL(void);
    void ReportState();
    void ReportAknEdStateEventL(TInt aDelay);
    void ReportAknEdStateEventL(MAknEdStateObserver::EAknEdwinStateEvent aEventType);
    
    TRect iSrcEditRect;
public:
    //CCoeControl接口
    void HandleCommand(TInt aCommand);
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
    void SizeChanged(void);
    TInt CountComponentControls() const;
    CCoeControl* ComponentControl(TInt aIndex) const;

    //MEikEdwinObserver
    void HandleEdwinEventL(CEikEdwin* aEdwin, MEikEdwinObserver::TEdwinEvent aEventType);
    
private:
    void InitTextEditor();
    void IntiSecretEditor();
    void InitMultTextEditor();
    void UpdateScrollbar();
    void CommitTextL(void);
    void DoCommitText(CNBrKernel* aNBK,const TDesC& aText);
    void Reset(void);
    
    static TInt OnDelay(TAny* aSelf);
    
    typedef enum _InputType
    {
        INPUTTYPE_NONE,
        INPUTTYPE_123,
        INPUTTYPE_PINYIN,
        INPUTTYPE_BIHUA,
        INPUTTYPE_abc,
        INPUTTYPE_Abc,
        INPUTTYPE_ABC,
    }InputType;
    InputType GetInputMode(void);
    
    MFEPControlObserver* iObserver;
    
    //CEikSecretEditor* iSecretEditor;
    CSecretEditor1* iSecretEditor;
    //CSecretEditor* iSecretEditor;
    CEikEdwin* iTextEditor;
    CEikEdwin* iMultTextEditor;

    TBool iSoftKeyDown;

    ControlType iControlType;
    CFont* iFont;
    TBool iOwnFont;
    TRgb iFontColor;

    TInt iMultEditor_MaxLine;
    TInt iMultEditor_ShowLine;
    TInt iMultEditor_FirstLine;

    TInt iTempInputState;
    CPeriodic* iDelay;
    TBool iActive;
    CNBrKernel* iActiveNBK;
    TBool iEditState;
    TInt iInitTextLen;
    CProbe* iProbe;
};


class CFEPHandler: public CBase
                 , public MObjectProvider
                 , public MCoeFepAwareTextEditor
                 , public MCoeFepAwareTextEditor_Extension1
                 //, public MAknEdStateObserver
                 //, private MCoeMessageMonitorObserver                 
{
public:
    CProbe* iProbe;
    
public:
    static CFEPHandler* NewL(MObjectProvider* aObjectProvider);
    ~CFEPHandler();
    
    void SetPage(void* aPage) { iPage = aPage; }
    void SetGdi(void* aGdi) { iGdi = aGdi; }
    void SetViewPort(void* aViewPort);
    void SetYOffset(TInt aY) { iYOffset = aY; }

    TCoeInputCapabilities InputCapabilities() const;
    void EnableEditing();
    void DisableEditing();
    TBool EditMode() const { return iEditState; }

    void UpdateFlagsState(TUint flags);
    void UpdateInputModeState(TUint inputMode, TUint permittedInputModes, TUint numericKeyMap);
    void UpdateCaseState(TUint currentCase, TUint permittedCase);
    void UpdateEditingMode();
    void CancelEditingMode();
    void HandleUpdateCursor();
    
private:
    //from MCoeMessageMonitorObserver
    //virtual void MonitorWsMessage(const TWsEvent& aEvent);
    
    // from MObjectProvider
    virtual TTypeUid::Ptr MopSupplyObject(TTypeUid aId);    
    
public: 
    
    // from MCoeFepAwareTextEditor
    void StartFepInlineEditL(
        const TDesC &aInitialInlineText,
        TInt aPositionOfInsertionPointInInlineText,
        TBool aCursorVisibility,
        const MFormCustomDraw *aCustomDraw,
        MFepInlineTextFormatRetriever &aInlineTextFormatRetriever,
        MFepPointerEventHandlerDuringInlineEdit &aPointerEventHandlerDuringInlineEdit);
    
    void UpdateFepInlineTextL(const TDesC& aNewInlineText, TInt aPositionOfInsertionPointInInlineText);
    void SetInlineEditingCursorVisibilityL(TBool aCursorVisibility);
    void CancelFepInlineEdit();
    TInt DocumentLengthForFep() const;
    TInt DocumentMaximumLengthForFep() const;
    void SetCursorSelectionForFepL(const TCursorSelection& aCursorSelection);
  
    void GetCursorSelectionForFep(TCursorSelection& aCursorSelection) const;
    void GetEditorContentForFep(TDes& aEditorContent, TInt aDocumentPosition, TInt aLengthToRetrieve) const;
    void GetFormatForFep(TCharFormat& aFormat, TInt aDocumentPosition) const;
    void GetScreenCoordinatesForFepL(TPoint& aLeftSideOfBaseLine, TInt& aHeight, TInt& aAscent, TInt aDocumentPosition) const;
    void DoCommitFepInlineEditL();
    MCoeFepAwareTextEditor_Extension1* Extension1(TBool &aSetToTrue);
    virtual void MCoeFepAwareTextEditor_Reserved_2();    

    // from MCoeFepAwareTextEditor_Extension1
    virtual void SetStateTransferingOwnershipL(CState* aState, TUid aTypeSafetyUid);   
    virtual CState* State(TUid aTypeSafetyUid);
    virtual void StartFepInlineEditL(TBool& aSetToTrue, const TCursorSelection& aCursorSelection, const TDesC& aInitialInlineText, TInt aPositionOfInsertionPointInInlineText, TBool aCursorVisibility, const MFormCustomDraw* aCustomDraw, MFepInlineTextFormatRetriever& aInlineTextFormatRetriever, MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit);
    virtual void SetCursorType(TBool& aSetToTrue, const TTextCursor& aTextCursor);
    
    void HandlePointerEventL(TPointerEvent& aEvent);
    //void HandleAknEdwinStateEventL (CAknEdwinState *aAknEdwinState, EAknEdwinStateEvent aEventType);
       
private: 
    CFEPHandler(MObjectProvider* aObjectProvider);
    void ConstructL(MObjectProvider* aObjectProvider);
    
public:	
    void SetState(CState* newVal);
    
private:
    void ClearText();
    void UpdateText(const TDesC& aInlineText);
    
private:    
	// Here is declared m_state 
    MCoeFepAwareTextEditor_Extension1::CState* m_state;
    
    MObjectProvider* iObjectProvider;
    TBool iEditState;
    void* iPage;
    void* iGdi;
    NRect iViewPort;
    TInt iYOffset;
    
    TUid mUID;
    HBufC* m_inlineEditText;
    int iWordCount; // 记录上次传入内核的字符总数
};

#endif // _PCTI_FEPHandler_h
