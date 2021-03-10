#include "../../../stdc/dom/event.h"
#include "../../../stdc/dom/page.h"
#include "../../../stdc/dom/document.h"
#include "../../../stdc/dom/view.h"
#include "../../../stdc/dom/node.h"
#include "../../../stdc/render/renderNode.h"
#include "../../../stdc/inc/nbk_cbDef.h"

#include "FEPHandler.h"
#include <aknedsts.h>
#include <aknedstsobs.h> 
#include <EIKENV.H>
#include <e32des16.h>
#include <avkon.hrh>
#include <aknutils.h>
#include "Probe.h"
#include "NBKPlatformData.h"
#include "NBrKernel.h"
#include "NbkGdi.h"
#include "ResourceManager.h"
#include <eikappui.h>
#include <barsread.h>

#define FEP_SYS_SECRET 1
#define MAX_PASSWORD_LEN    128

void NBK_fep_enable(void* pfd)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* nbk = (CNBrKernel*)p->nbk;

#if ENABLE_FEP
    CFEPHandler* fep = (CFEPHandler*)p->fep;
    fep->SetGdi(p->gdi);
    
    NRect viewPort;
    NPoint offset;
    nbk->GetViewPort(&viewPort);
    fep->SetViewPort(&viewPort);
    nbk->GetNBKOffset(&offset);
    fep->SetYOffset(offset.y);
    fep->EnableEditing();
#else
    CFEPControl* fepControl = CResourceManager::GetPtr()->GetFEPControl();
    fepControl->SetEditState(nbk, ETrue);
#endif
}

void NBK_fep_disable(void* pfd)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* nbk = (CNBrKernel*)p->nbk;
    
#if ENABLE_FEP
    CFEPHandler* fep = (CFEPHandler*)p->fep;
    fep->DisableEditing();
#else
    CFEPControl* fepControl = CResourceManager::GetPtr()->GetFEPControl();
    if (fepControl->IsActive())
        fepControl->DeActive();
    fepControl->SetEditState(nbk, EFalse);
#endif
}

void NBK_fep_updateCursor(void* pfd)
{
    NBK_PlatformData* p = (NBK_PlatformData*)pfd;
    CNBrKernel* nbk = (CNBrKernel*)p->nbk; 

#if ENABLE_FEP
    CFEPHandler* fep = (CFEPHandler*) p->fep;
    fep->HandleUpdateCursor();
#else
    CFEPControl* fepControl = CResourceManager::GetPtr()->GetFEPControl();
    if (fepControl->IsActive())
        fepControl->SetSelection(nbk);
#endif
}

CFEPHandler::CFEPHandler(MObjectProvider* aObjectProvider) :
    m_inlineEditText(NULL)
{
    iObjectProvider = aObjectProvider;
    m_state = NULL;
    mUID = TUid::Null();
    //iWordCount = 0;
}

TCoeInputCapabilities CFEPHandler::InputCapabilities() const 
{
    if (iEditState) {
        TUint caps = TCoeInputCapabilities::ENavigation;
        CAknEdwinState* state = (CAknEdwinState*)m_state;
        
//        caps |= TCoeInputCapabilities::EWesternNumericIntegerPositive
//                | TCoeInputCapabilities::EWesternNumericIntegerNegative
//                | TCoeInputCapabilities::EAllText;
        
        if (state->PermittedInputModes() == EAknEditorNumericInputMode) {
            caps |= TCoeInputCapabilities::EWesternNumericIntegerPositive |
                    TCoeInputCapabilities::EWesternNumericIntegerNegative;
        }
        else {
            caps |= TCoeInputCapabilities::EAllText;        
        }
        
        return TCoeInputCapabilities(caps, const_cast<CFEPHandler*>(this), NULL);
    }
    else {
        TCoeInputCapabilities emptyCaps(TCoeInputCapabilities::ENone);
        return emptyCaps;
    }
}

CFEPHandler::~CFEPHandler()
{
    if (m_state) {
        delete m_state;
        m_state = 0;
    }
    if (m_inlineEditText) {
        delete m_inlineEditText;
        m_inlineEditText = NULL;
    }

    //(CEikonEnv::Static())->RemoveMessageMonitorObserver(*this);
}

CFEPHandler* CFEPHandler::NewL(MObjectProvider* aObjectProvider)
{
	CFEPHandler *ret = new CFEPHandler(aObjectProvider);
    ret->ConstructL(aObjectProvider);
    return ret;
}

void CFEPHandler::ConstructL(MObjectProvider* iObjectProvider)
{
    CAknEdwinState *state = new (ELeave) CAknEdwinState();
    state->SetObjectProvider(iObjectProvider);

    //state->SetFlags(EEikEdwinNoHorizScrolling | EEikEdwinResizable);
    state->SetFlags(
        EEikEdwinNoHorizScrolling |
        EEikEdwinResizable |
        EEikEdwinAlternativeWrapping |
        EEikEdwinAllowUndo |
        EEikEdwinAutoSelection);
    // There are defined allowed input modes
    state->SetPermittedInputModes(/*EAknEditorTextInputMode*/EAknEditorNumericInputMode);
    // There is set default input modje
    state->SetDefaultInputMode(/*EAknEditorTextInputMode*/EAknEditorNumericInputMode);

    // Here is defined numeric keymap
    //state->SetNumericKeymap(EAknEditorStandardNumberModeKeymap);

    m_state = state;

    //(CEikonEnv::Static())->AddMessageMonitorObserverL(*this); //add msg observer
}

void CFEPHandler::EnableEditing()
{
//    if (iEditState)
//        return;
    
    CCoeFep* fep = CCoeEnv::Static()->Fep();
    
    if (fep) {
#if FEP_TEST > 1
        iProbe->OutputTime();
        iProbe->OutputChar(">>> EnableEditing", -1);
        iProbe->OutputReturn();
#endif

        iEditState = ETrue;
        CCoeEnv::Static()->InputCapabilitiesChanged();

        CancelEditingMode();
        UpdateEditingMode();
        fep->HandleChangeInFocus();

        CAknEdwinState* edwinState =
            STATIC_CAST(CAknEdwinState*, const_cast<CFEPHandler*>(this)->State(KNullUid));
        edwinState->ReportAknEdStateEventL(MAknEdStateObserver::EAknActivatePenInputRequest);
    }
}

void CFEPHandler::DisableEditing()
{    
    if (!iEditState)
        return;
    
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("<<< DisableEditing", -1);
    iProbe->OutputReturn();
#endif
    
    iEditState = EFalse;
    CancelEditingMode();
    CCoeEnv::Static()->InputCapabilitiesChanged();
    
//    CAknEdwinState* state = static_cast<CAknEdwinState*>(State(KNullUid));
//    if ( state ) {
//        TRAP_IGNORE( state->ReportAknEdStateEventL(MAknEdStateObserver::EAknSyncEdwinState ) );
//    }
//    CommitFepInlineEditL(*CEikonEnv::Static());
}

void CFEPHandler::StartFepInlineEditL(
    const TDesC& aInitialInlineText,
    TInt aPositionOfInsertionPointInInlineText,
    TBool aCursorVisibility,
    const MFormCustomDraw* aCustomDraw,
    MFepInlineTextFormatRetriever& aInlineTextFormatRetriever,
    MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit)
{
//    if (doc_IsEditorFull(iPage))
//        return;
    
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("StartFepInlineEditL", -1);
    iProbe->Output(aInitialInlineText);
    iProbe->OutputInt(aPositionOfInsertionPointInInlineText);
    //iProbe->OutputInt(iWordCount);
    iProbe->OutputReturn();
#endif
    
    CCoeEnv::Static()->ForEachFepObserverCall(FepObserverHandleStartOfTransactionL);
    ClearText();
    UpdateText(aInitialInlineText);
}

void CFEPHandler::UpdateFepInlineTextL(
    const TDesC& aNewInlineText, TInt aPositionOfInsertionPointInInlineText)
{    
//    if (doc_IsEditorFull(iPage))
//        return;
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("UpdateFepInlineTextL", -1);
    iProbe->Output(aNewInlineText);
    iProbe->OutputInt(aPositionOfInsertionPointInInlineText);
    //iProbe->OutputInt(iWordCount);
    iProbe->OutputReturn();
#endif
    
    ClearText();
    UpdateText(aNewInlineText);
}

// FEP编辑确定，提交输入结果
void CFEPHandler::DoCommitFepInlineEditL()
{
    //iWordCount = 0;
//    if (doc_IsEditorFull(iPage))
//        return;
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("DoCommitFepInlineEditL", -1);
    iProbe->Output(*m_inlineEditText);
    //iProbe->OutputInt(iWordCount);
    iProbe->OutputReturn();
#endif
    
    ClearText();
    
    if (m_inlineEditText && DocumentLengthForFep() < DocumentMaximumLengthForFep()) {
        NEvent event;
        NBK_memset(&event, 0, sizeof(NEvent));
        event.type = NEEVENT_KEY;
        event.page = iPage;
        event.d.keyEvent.key = NEKEY_CHAR;
        for (int i = 0; i < m_inlineEditText->Length(); i++) {
            event.d.keyEvent.chr = (*m_inlineEditText)[i];
            doc_processKey(&event);
        }
    }
    
    //delete the m_inlineEditText since text is commited
    delete m_inlineEditText;
    m_inlineEditText = NULL;
    
    HandleUpdateCursor();
    UpdateEditingMode();

}

void CFEPHandler::CancelFepInlineEdit()
{
    ClearText();
    
    if(m_inlineEditText)
    {
        delete m_inlineEditText;
        m_inlineEditText = NULL;
    }
    
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("CancelFepInlineEdit", -1);
    //iProbe->OutputInt(iWordCount);
    iProbe->OutputReturn();
#endif
}

// 获取当前可编辑文本的长度
TInt CFEPHandler::DocumentLengthForFep() const
{
    NPage* page = (NPage*) iPage;
    return node_getEditLength((NNode*) view_getFocusNode(page->view));
}

// 获取可编辑文本的最大长度
TInt CFEPHandler::DocumentMaximumLengthForFep() const
{
    NPage* page = (NPage*)iPage;
    return node_getEditMaxLength((NNode*)view_getFocusNode(page->view));
}

// 设置选择位置
void CFEPHandler::SetCursorSelectionForFepL(const TCursorSelection& aCursorSelection)
{    
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("SetCursorSelectionForFepL: ", -1);
    iProbe->OutputInt(aCursorSelection.iAnchorPos);
    iProbe->OutputInt(aCursorSelection.iCursorPos);
    iProbe->OutputReturn();
#endif
    
    NPage* page = (NPage*)iPage;
    NNode* focus = (NNode*)view_getFocusNode(page->view);

    if (focus) {
        NRenderNode* r = (NRenderNode*)focus->render;
        if (r)
            renderNode_setSelPos(r, aCursorSelection.iAnchorPos, aCursorSelection.iCursorPos);
        
        HandleUpdateCursor();      
        nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
    }
}

// 获取选择位置
void CFEPHandler::GetCursorSelectionForFep(TCursorSelection& aCursorSelection) const
{
    aCursorSelection.iAnchorPos = 0;
    aCursorSelection.iCursorPos = 0;

    NPage* page = (NPage*)iPage;
    NNode* focus = (NNode*)view_getFocusNode(page->view);
    
    if (focus) {
        NRenderNode* r = (NRenderNode*)focus->render;
        if (r) {
            int start, end;
            renderNode_getSelPos(r, &start, &end);
            aCursorSelection.iAnchorPos = start;
            aCursorSelection.iCursorPos = end;
        }
    }
    
#if FEP_TEST > 2
    iProbe->OutputTime();
    iProbe->OutputChar("GetCursorSelectionForFep: ", -1);
    iProbe->OutputInt(aCursorSelection.iAnchorPos);
    iProbe->OutputInt(aCursorSelection.iCursorPos);
    iProbe->OutputReturn();
#endif
}

// 获取指定的编辑文本
void CFEPHandler::GetEditorContentForFep(
    TDes& aEditorContent, TInt aDocumentPosition, TInt aLengthToRetrieve) const
{
#if FEP_TEST > 1
    iProbe->OutputTime();
    iProbe->OutputChar("GetEditorContentForFep", -1);
    iProbe->OutputInt(aDocumentPosition);
    iProbe->OutputInt(aLengthToRetrieve);
    iProbe->OutputReturn();
#endif
    
    aEditorContent = KNullDesC;
    if (aLengthToRetrieve <= 0)
        return;
    
    NPage* page = (NPage*)iPage;
    NNode* focus = (NNode*)view_getFocusNode(page->view);
    
    if (focus && aDocumentPosition >= 0) {
        NRenderNode* r = (NRenderNode*)focus->render;
        if (r) {
            wchr* text;
            int len;
            text = renderNode_getEditText16(r, &len);
            if (text) {
                TPtrC16 str(text, len);
                if (aDocumentPosition >= str.Length())
                    aDocumentPosition = str.Length() - aLengthToRetrieve - 1;
                if (aDocumentPosition >= 0) {
                    TPtrC16 subStr = str.Mid(aDocumentPosition, aLengthToRetrieve);
                    aEditorContent.Copy(subStr);
                }
            }
        }
    }
}

void CFEPHandler::GetFormatForFep(TCharFormat& aFormat, TInt aDocumentPosition) const
{
}

// 获取FEP屏幕显示位置
void CFEPHandler::GetScreenCoordinatesForFepL(
    TPoint& aLeftSideOfBaseLine, TInt& aHeight, TInt& aAscent, TInt aDocumentPosition) const
{
    NNode* focus = (NNode*)view_getFocusNode(((NPage*)iPage)->view);
    if (focus) {
        NRect r, vr;
        NFontId fid;
        if (renderNode_getRectOfEditing((NRenderNode*)focus->render, &r, &fid)) {
            CNbkGdi* gdi = (CNbkGdi*)iGdi;
            aHeight = gdi->GetFontHeight(fid);
            aAscent = gdi->GetFontAscent(fid);
            aDocumentPosition = renderNode_getEditLength((NRenderNode*)focus->render);
            vr.l = r.l - iViewPort.l;
            vr.t = r.t - iViewPort.t;
            vr.r = vr.l + rect_getWidth(&r);
            vr.b = vr.t + rect_getHeight(&r);
            
            aLeftSideOfBaseLine.SetXY(vr.l, vr.b + iYOffset);
        }
    }
    aAscent = 0;
    aHeight = 0;
}

void CFEPHandler::SetInlineEditingCursorVisibilityL(TBool aCursorVisibility)
{
}

MCoeFepAwareTextEditor_Extension1* CFEPHandler::Extension1(TBool &aSetToTrue)
{
    aSetToTrue = ETrue;
    return STATIC_CAST(MCoeFepAwareTextEditor_Extension1*, this);
}

void CFEPHandler::MCoeFepAwareTextEditor_Reserved_2()
{
}

void CFEPHandler::SetStateTransferingOwnershipL(CState* aState, TUid aTypeSafetyUid)
{
    if (m_state) {
        delete m_state;
        m_state = 0;
    }
    m_state = aState;
    mUID = aTypeSafetyUid;
}

MCoeFepAwareTextEditor_Extension1::CState* CFEPHandler::State(TUid /*aTypeSafetyUid*/)
{
    if (!m_state) {
        CAknEdwinState* state = new CAknEdwinState();
        state->SetObjectProvider(iObjectProvider);
        m_state = state;
    }
    return m_state;
}

void CFEPHandler::StartFepInlineEditL(TBool& aSetToTrue, const TCursorSelection& aCursorSelection,
    const TDesC& aInitialInlineText, TInt aPositionOfInsertionPointInInlineText,
    TBool aCursorVisibility, const MFormCustomDraw* aCustomDraw,
    MFepInlineTextFormatRetriever& aInlineTextFormatRetriever,
    MFepPointerEventHandlerDuringInlineEdit& aPointerEventHandlerDuringInlineEdit)
{
    aSetToTrue = ETrue;
    SetCursorSelectionForFepL(aCursorSelection);
    
    StartFepInlineEditL(aInitialInlineText, aPositionOfInsertionPointInInlineText,
        aCursorVisibility, aCustomDraw, aInlineTextFormatRetriever,
        aPointerEventHandlerDuringInlineEdit);
}

void CFEPHandler::SetCursorType(TBool& /*aSetToTrue*/, const TTextCursor& /*aTextCursor*/)
{
}

void CFEPHandler::HandlePointerEventL(TPointerEvent& aEvent)
{
    if (TPointerEvent::EButton1Up == aEvent.iType) {
        CAknEdwinState* edwinState =
            STATIC_CAST(CAknEdwinState*, const_cast<CFEPHandler*>(this)->State(KNullUid));
        edwinState->ReportAknEdStateEventL(MAknEdStateObserver::EAknActivatePenInputRequest);
    }

}

//void CFEPHandler::HandleAknEdwinStateEventL(
//    CAknEdwinState *aAknEdwinState, EAknEdwinStateEvent aEventType)
//{
//}

//void CFEPHandler::MonitorWsMessage(const TWsEvent &aEvent)
//{
//    if (aEvent.Type() == EEventPointer)
//        HandlePointerEventL(*aEvent.Pointer());
//}

TTypeUid::Ptr CFEPHandler::MopSupplyObject(TTypeUid aId)
{
    return aId.Null();
}

void CFEPHandler::HandleUpdateCursor()
{
    CAknEdwinState* edwinState = \
        STATIC_CAST(CAknEdwinState*, const_cast<CFEPHandler*>(this)->State(KNullUid));
    edwinState->ReportAknEdStateEventL(MAknEdStateObserver::EAknCursorPositionChanged);
}

void CFEPHandler::UpdateFlagsState(TUint flags)
{
    CAknEdwinState* state = static_cast<CAknEdwinState*> (State(KNullUid));

    state->SetFlags(flags | EAknEditorFlagUseSCTNumericCharmap);
    state->ReportAknEdStateEventL(MAknEdStateObserver::EAknEdwinStateFlagsUpdate);
}

void CFEPHandler::UpdateInputModeState(TUint inputMode, TUint permittedInputModes, TUint numericKeyMap)
{
    CAknEdwinState* state = static_cast<CAknEdwinState*> (State(KNullUid));

    if (permittedInputModes != EAknEditorNumericInputMode) {
        EVariantFlag variant = AknLayoutUtils::Variant();
        if (variant == EApacVariant) {
            permittedInputModes |= EAknEditorTextInputMode | EAknEditorHalfWidthTextInputMode
                | EAknEditorFullWidthTextInputMode | EAknEditorKatakanaInputMode
                | EAknEditorFullWidthKatakanaInputMode | EAknEditorHiraganaKanjiInputMode
                | EAknEditorHiraganaInputMode;
        }
    }

    state->SetDefaultInputMode(inputMode);
    state->SetCurrentInputMode(inputMode);
    state->SetPermittedInputModes(permittedInputModes);
    state->SetNumericKeymap(static_cast<TAknEditorNumericKeymap> (numericKeyMap));
    state->ReportAknEdStateEventL(MAknEdStateObserver::EAknSyncEdwinState);
    state->ReportAknEdStateEventL(MAknEdStateObserver::EAknEdwinStateInputModeUpdate);
}

void CFEPHandler::UpdateCaseState(TUint currentCase, TUint permittedCase)
{
    CAknEdwinState* state = static_cast<CAknEdwinState*> (State(KNullUid));

    state->SetDefaultCase(currentCase);
    state->SetCurrentCase(currentCase);
    state->SetPermittedCases(permittedCase);
    state->ReportAknEdStateEventL(MAknEdStateObserver::EAknEdwinStateCaseModeUpdate);
}

void CFEPHandler::UpdateEditingMode()
{
//    TUint currentCase(EAknEditorLowerCase);
//    TUint permittedCase(EAknEditorAllCaseModes);
//    TUint inputMode(EAknEditorTextInputMode);
//    TUint permittedInputModes(EAknEditorAllInputModes);
//    TUint flags(EAknEditorFlagDefault);
//    TUint numericKeyMap(EAknEditorStandardNumberModeKeymap);

    CAknEdwinState* state = static_cast<CAknEdwinState*>(State(KNullUid));
    if (state) {
        state->SetDefaultCase(EAknEditorLowerCase);
        state->SetPermittedInputModes(EAknEditorAllInputModes);
        state->SetPermittedCases(EAknEditorAllCaseModes);//allow everything
    }
}

void CFEPHandler::CancelEditingMode()
{
    if (m_inlineEditText) {
        delete m_inlineEditText;
        m_inlineEditText = NULL;
    }
    
    UpdateInputModeState(EAknEditorNullInputMode, EAknEditorAllInputModes, EAknEditorStandardNumberModeKeymap);
    UpdateFlagsState(EAknEditorFlagDefault);
    UpdateCaseState(EAknEditorLowerCase, EAknEditorAllCaseModes);
    CancelFepInlineEdit();
}

// 清除上次传入的字符，已无效
void CFEPHandler::ClearText()
{
    TInt deleteCnt = m_inlineEditText ? m_inlineEditText->Length() : 0;
    
    NEvent event;
    NBK_memset(&event, 0, sizeof(NEvent));
    event.type = NEEVENT_KEY;
    event.page = iPage;
    event.d.keyEvent.key = NEKEY_BACKSPACE;
    
    NPage* page = (NPage*)iPage;
    NNode* focus = (NNode*)view_getFocusNode(page->view);
    
    if (focus && deleteCnt) {
        NRenderNode* r = (NRenderNode*) focus->render;
        if (r) {
            int start, end;
            renderNode_getSelPos(r, &start, &end);
            TInt nSelTextCnt = end > start ? end - start : start - end;

            if(nSelTextCnt)
                deleteCnt = deleteCnt - nSelTextCnt + 1;
            for (int i = 0; i < deleteCnt; i++)
                doc_processKey(&event);
        }
    }

#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("ClearText", -1);
    iProbe->OutputInt(deleteCnt);
    iProbe->OutputReturn();
#endif
    //iWordCount = 0;
}

// 将新产生的字符传入内核编辑器
void CFEPHandler::UpdateText(const TDesC& aInlineText)
{
#if FEP_TEST
    iProbe->OutputTime();
    iProbe->OutputChar("UpdateText", -1);
    iProbe->Output(aInlineText);
    iProbe->OutputReturn();
#endif
    
    if (m_inlineEditText) {
        delete m_inlineEditText;
        m_inlineEditText = NULL;
    }
    
    m_inlineEditText = aInlineText.Alloc();

    NEvent event;
    NBK_memset(&event, 0, sizeof(NEvent));
    event.type = NEEVENT_KEY;
    event.page = iPage;
    event.d.keyEvent.key = NEKEY_CHAR;
    for (int i = 0; i < aInlineText.Length(); i++) {
        event.d.keyEvent.chr = aInlineText[i];
        doc_processKey(&event);
    }
    //iWordCount = aInlineText.Length(); // 记录传入的字符量，该字符可能无效
}

void CFEPHandler::SetViewPort(void* aViewPort)
{
    NBK_memcpy(&iViewPort,aViewPort,sizeof(NRect));
}

CSecretEditor::CSecretEditor():iCursorDelay(NULL)
{
    iPassword = NULL;
}

CSecretEditor::~CSecretEditor()
{
    if (iPassword) {
        delete iPassword;
        iPassword = NULL;
    }
    if(iDisplayPassword)
    {
        delete iDisplayPassword;
        iDisplayPassword = NULL;
    }
//    if(iTextOnEdit)
//    {
//        delete iTextOnEdit;
//        iTextOnEdit = NULL;
//    }
    
    if (iCursorDelay) {
        if (iCursorDelay->IsActive())
            iCursorDelay->Cancel();

        delete iCursorDelay;
        iCursorDelay = NULL;
    }
}

void CSecretEditor::ConstructL(TInt aCount)
{
    iCount = aCount;
    CEikEdwin::ConstructL(EAknEditorFlagDefault | EEikEdwinNoAutoSelection
        | EEikEdwinJustAutoCurEnd, 0, aCount, 1);
    //CEikEdwin::SetReadOnly(ETrue);
    SetAknEditorAllowedInputModes(EAknEditorSecretAlphaInputMode);
    //SetAknEditorAllowedInputModes(EAknEditorHalfWidthTextInputMode);
    SetAknEditorInputMode(EAknEditorSecretAlphaInputMode);
    EnableCcpuSupportL(EFalse);
    iPassword = HBufC::NewL(aCount);
    iDisplayPassword = HBufC::NewL(aCount);
    //iTextOnEdit = HBufC::NewL(aCount);
}

void CSecretEditor::Draw(const TRect& aRect) const
{
    //CEikEdwin::Draw(aRect);
    CWindowGc& gc = SystemGc();
    gc.SetClippingRect(aRect);
    gc.Clear(aRect);
    TRect r = Rect();
    TInt YOffset = 0;
    TInt txtWidth = 0;
    TInt XOffset = 0;
    TInt space = 2;
    TInt totalWidth;
    TSize cursorSize(3, aRect.Height() - 4);
    if (iFont) {
        TInt fontHeight = iFont->FontMaxHeight();
        cursorSize.iHeight = fontHeight;
        YOffset = (r.Height() - fontHeight) / 2;
        TInt ascent = iFont->FontMaxAscent();
        txtWidth = iFont->TextWidthInPixels(*iDisplayPassword);
        r.iTl.iY = YOffset + ascent;
        totalWidth = txtWidth + cursorSize.iWidth + space;
        if (totalWidth >= aRect.Width()) {
            XOffset = txtWidth - totalWidth;
//            gc.CancelClippingRect();
//            TRect clip(aRect);
//            clip.iBr.iX -= space*3;
//            gc.SetClippingRect(clip);
        }
        r.iTl.iX -= XOffset;
        gc.UseFont(iFont);
    }
    gc.DrawText(*iDisplayPassword, r.iTl);
    //gc.CancelClippingRect();
    if (iShowCursor) {
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(TRgb(0,0,0));
        TInt curPos = CursorPos();
        if (curPos) {
            TPtrC txeBeforeCurosr = iDisplayPassword->Left(curPos);
            txtWidth = iFont->TextWidthInPixels(txeBeforeCurosr);
            totalWidth = txtWidth + cursorSize.iWidth + space;
        }
        else
            txtWidth = 0;
        
        TInt ax = aRect.iTl.iX + txtWidth;
        if (totalWidth >= aRect.Width())
            ax = aRect.iBr.iX - space - cursorSize.iWidth;
        TInt ay = aRect.iTl.iY+YOffset;
        TInt bx = ax + cursorSize.iWidth;
        TInt by = ay+ cursorSize.iHeight;
        TRect cursorRect(ax,ay,bx,by);
        //gc.SetClippingRect(cursorRect);
        gc.Clear(cursorRect);
        gc.Reset();
    }
    gc.CancelClippingRect();
}

TKeyResponse CSecretEditor::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
    TInt pos = CursorPos();
    
    switch (aKeyEvent.iCode) {
    case EKeyBackspace:
    case EKeyDelete:
    {
//        if (aType == EEventKey) {
//            UpdateDisplayPassword();
//            TBool backspace = ETrue;
//            if (EKeyDelete == aKeyEvent.iCode)
//                backspace = EFalse;
//            OnDeletePassword(pos, backspace);
//        }
        return CEikEdwin::OfferKeyEventL(aKeyEvent, aType);
    }
    case EKeyLeftArrow:
    case EKeyRightArrow:
        return CEikEdwin::OfferKeyEventL(aKeyEvent, aType);

    default:
    {
        if (aKeyEvent.iCode >= 0x20 && aKeyEvent.iCode <= ENonCharacterKeyCount) {

            CEikEdwin::OfferKeyEventL(aKeyEvent, aType);
            TInt n = TextLength();
            SetCursorPosL(TextLength(), EFalse);
//            TKeyEvent keyEvent;
//            keyEvent.iRepeats = 0;
//            keyEvent.iModifiers = 262144;
//            keyEvent.iScanCode = 0;
//            keyEvent.iCode = 42;
//            CEikEdwin::OfferKeyEventL(keyEvent, aType);
//            if (aType == EEventKey) {
//
//                //                _LIT(KDisplayChar,"*");
//                //                TCursorSelection select;
//                //                select.iAnchorPos = pos;
//                //                select.iCursorPos = pos;
//                //                InsertDeleteCharsL(pos, KDisplayChar, select);
//
//                pos = CursorPos();
//                TBufC<1> newChar;
//                newChar.Des().Append(aKeyEvent.iCode);
//                OnInsertPassword(newChar, iCursorPos);
//                ConvertPassword();
//                return EKeyWasConsumed;
//            }
        }   
        return EKeyWasConsumed;
        break;
    }
    }
    return EKeyWasConsumed;
}

void CSecretEditor::GetPassword(TDes& aDes) const
{
    aDes.Copy(*iPassword);
}

void CSecretEditor::SetPassword(const TDesC& aTxt)
{
    TInt len = aTxt.Length();
    if (aTxt.Length() > MAX_PASSWORD_LEN)
        len = MAX_PASSWORD_LEN;
    iPassword->Des().Copy(aTxt.Ptr(), len);
    //iCursorPos = iPassword->Length();
    ConvertPassword();
}

void CSecretEditor::HideControlText(void)
{ 
//    iTextOnEdit->Des().Zero();
//    TPtr text = iTextOnEdit->Des();
//    GetText(text);    
//    
//    TInt newLen = text.Length();
//    TInt oldLen = iPassword->Length();
//    TInt curPos = CursorPos();
//    if (newLen && newLen > oldLen) { //插入字符
//        
//        CProbe* probe = CResourceManager::GetPtr()->GetProbe();
//        probe->OutputChar("CSecretEditor::HideControlText iCursorPos =",-1);
//        probe->OutputInt(iCursorPos);
//        probe->OutputReturn();
//        
//        TInt pos = curPos - (newLen - oldLen);
//        if(pos >= 0)
//        {
//            TPtrC newChars = text.Mid(pos, newLen - oldLen);
//            probe->OutputChar("CSecretEditor::HideControlText newChars =", -1);
//            probe->Output(newChars);
//            probe->OutputReturn();
//            OnInsertPassword(newChars, iCursorPos);
//        }
//    }
//    else if (oldLen && newLen < oldLen)//删除字符
//    {
//        //Backspace
//        if (iCursorPos > curPos) {
//            OnDeletePassword(curPos, ETrue);
//        }
//        else if (iCursorPos == curPos) { //Delete
//            OnDeletePassword(curPos, EFalse);
//        }
//    }
   
    iPassword->Des().Zero();
    TPtr text = iPassword->Des();
    GetText(text);
    ConvertPassword();
}

void CSecretEditor::UpdateCursor(void)
{
//    TInt pos = CursorPos();
//    if (pos >= 0 && pos <= iPassword->Length())
//        iCursorPos = pos;
}

void CSecretEditor::ConvertPassword(void)
{    
    _LIT(KDisplayChar,"*");
    iDisplayPassword->Des().Zero();
    for (TInt i = 0; i < iPassword->Length(); i++) {
        iDisplayPassword->Des().Append(KDisplayChar);
    }
}

void CSecretEditor::UpdateDisplayPassword(void)
{
//    iDisplayPassword->Des().Zero();
//    TPtr tmp = iDisplayPassword->Des();
//    GetText(tmp);
}

void CSecretEditor::OnInsertPassword(const TDesC& aNewChars, TInt aCurosrPos)
{    
    _LIT(KDisplayChar,"*");
    if (aNewChars != KDisplayChar) { //插入非*号字符时保存输入字符
        HBufC* buf1 = NULL;
        HBufC* buf2 = NULL;
        TInt oldLen = iPassword->Length();
        if (aCurosrPos > 0) {
            TPtrC tmp = iPassword->Left(aCurosrPos);
            if (tmp.Length())
                buf1 = tmp.AllocL();
        }
        if (oldLen - aCurosrPos > 0) {
            TPtrC tmp = iPassword->Right(oldLen - aCurosrPos);
            if (tmp.Length())
                buf1 = tmp.AllocL();
        }

        iPassword->Des().Zero();
        if (buf1) {
            iPassword->Des().Append(*buf1);
            delete buf1;
        }
        iPassword->Des().Append(aNewChars);
        if (buf2) {
            iPassword->Des().Append(*buf2);
            delete buf2;
        }
    }
    
    UpdateCursor();
}

void CSecretEditor::OnDeletePassword(TInt curPos, TBool aBackspace)
{
    TInt len = iPassword->Length();

    HBufC* buf1 = NULL;
    HBufC* buf2 = NULL;
    if (aBackspace) {
        if (curPos > 0) {
            {
                TPtrC tmp = iPassword->Left(curPos);
                if (tmp.Length())
                    buf1 = tmp.AllocL();
            }
        }
        if (len - curPos - 1 > 0) {
            TPtrC tmp = iPassword->Right(len - curPos - 1);
            if (tmp.Length())
                buf2 = tmp.AllocL();
        }
    }
    else {
        if (curPos) {
            TPtrC tmp = iPassword->Left(curPos);
            if (tmp.Length())
                buf1 = tmp.AllocL();
        }
        if (len - curPos - 1 > 0)
        {
            TPtrC tmp = iPassword->Right(len - curPos - 1);
            if (tmp.Length())
                buf2 = tmp.AllocL();
        }
    }

    iPassword->Des().Zero();
    if (buf1) {
        iPassword->Des().Append(*buf1);
        delete buf1;
    }
    if (buf2) {
        iPassword->Des().Append(*buf2);
        delete buf2;
    }
    
    UpdateCursor();
}

void CSecretEditor::StartCursorL(TInt aDelay)
{
    if (!iCursorDelay)
        iCursorDelay = CPeriodic::NewL(CPeriodic::EPriorityUserInput);
    
    if (iCursorDelay && iCursorDelay->IsActive())
        iCursorDelay->Cancel();

    iCursorDelay->Start(aDelay, 600000, TCallBack(OnUpdateCurosr, this));
}

void CSecretEditor::StopCursor(void)
{
    if (iCursorDelay && iCursorDelay->IsActive()) {
        iCursorDelay->Cancel();
        delete iCursorDelay;
        iCursorDelay = NULL;
    }
}

TInt CSecretEditor::OnUpdateCurosr(TAny* aSelf)
{
    CSecretEditor& self = *((CSecretEditor*) aSelf);
    self.iShowCursor = !self.iShowCursor;
    CResourceManager::GetPtr()->GetFEPControl()->DrawNow();
    return 0;
}

void CSecretEditor::DoUpdateCursor(void)
{
    CWindowGc& gc = SystemGc();
    TRect r = Rect();
    
    TInt YOffset = 0;
    TInt txtWidth = 0;
    TInt XOffset = 0;
    TInt space = 2;
    TInt totalWidth = 0;
    TSize cursorSize(3, r.Height() - 4);
    if (iFont) {
        TInt fontHeight = iFont->FontMaxHeight();
        cursorSize.iHeight = fontHeight;
        YOffset = (r.Height() - fontHeight) / 2;
//        TInt ascent = iFont->FontMaxAscent();
//        txtWidth = iFont->TextWidthInPixels(*iDisplayPassword);
//        r.iTl.iY = YOffset + ascent;
//        totalWidth = txtWidth + cursorSize.iWidth + space;
//        if (totalWidth >= aRect.Width()) {
//            XOffset = txtWidth - totalWidth;
//            //            gc.CancelClippingRect();
//            //            TRect clip(aRect);
//            //            clip.iBr.iX -= space*3;
//            //            gc.SetClippingRect(clip);
//        }
//        r.iTl.iX -= XOffset;
//        gc.UseFont(iFont);
    }
    //gc.DrawText(*iDisplayPassword, r.iTl);
    //gc.CancelClippingRect();
    if (iShowCursor) {
        gc.SetBrushColor(TRgb(0, 0, 0));
        TInt curPos = CursorPos();
        if (curPos) {
            TPtrC txeBeforeCurosr = iDisplayPassword->Left(curPos);
            txtWidth = iFont->TextWidthInPixels(txeBeforeCurosr);
            totalWidth = txtWidth + cursorSize.iWidth + space;
        }
        else
            txtWidth = 0;

        TInt ax = r.iTl.iX + txtWidth;
        if (totalWidth >= r.Width())
            ax = r.iBr.iX - space - cursorSize.iWidth;
        TInt ay = r.iTl.iY + YOffset;
        TInt bx = ax + cursorSize.iWidth;
        TInt by = ay + cursorSize.iHeight;
        TRect cursorRect(ax, ay, bx, by);
        gc.SetClippingRect(cursorRect);
        gc.Clear(cursorRect);
        gc.CancelClippingRect();
    }
}

CSecretEditor1::CSecretEditor1()
{

}

CSecretEditor1::~CSecretEditor1()
{

}

void CSecretEditor1::ConstructL(void)
{
    //for CEikSecrectEditor        
    TInt16 num_letters = MAX_PASSWORD_LEN;
    TPckgC<TInt16> dummy(num_letters);
    TResourceReader reader;
    reader.SetBuffer(&dummy);
    CEikSecretEditor::ConstructFromResourceL(reader);
    SetDefaultInputMode(EAknEditorSecretAlphaInputMode | EAknEditorNumericInputMode);
    SetSkinBackgroundControlContextL(NULL);
    SetBorder(TGulBorder::ENone); //无边框
    TBool b = HasBorder();
    TGulBorder boder = Border();
    //TMargins m = boder.Margins();
    SetAdjacent(EGulAdjBottom);
    RevealSecretText(EFalse);
}

void CSecretEditor1::SetFont(CFont* aFont)
{
    iCustomFont = aFont;
    AknSetFont(*aFont);
}

void CSecretEditor1::Draw(const TRect& aRect) const
{      
    CWindowGc& gc = SystemGc();
    gc.SetClippingRect(aRect);
    gc.Clear(aRect);
    TRect r = Rect();
    if (iCustomFont) {
        
        TInt XOffset = 0;
        TInt fontHeight = iCustomFont->FontMaxHeight();
        TInt YOffset = (r.Height() - fontHeight) / 2;
        TInt txtWidth = iCustomFont->TextWidthInPixels(iBuf);
        r.iTl.iY = YOffset + iCustomFont->FontMaxAscent();
        TInt totalWidth = txtWidth + 2;
        if (totalWidth >= aRect.Width()) {
            XOffset = txtWidth - totalWidth;
        }
        r.iTl.iX -= XOffset;
        gc.UseFont(iCustomFont);
        gc.DrawText(iBuf, r.iTl);
    }
    gc.CancelClippingRect();
}

//DXSysEditor
CFEPControl::CFEPControl() :
    iOwnFont(EFalse), iActive(EFalse), iEditState(EFalse), iActiveNBK(NULL)
{

}

CFEPControl::~CFEPControl()
{
    if (IsActive())
        DeActive();

    if (iDelay) {
        if (iDelay->IsActive())
            iDelay->Cancel();

        delete iDelay;
        iDelay = NULL;
    }
    
    SetControlFocus(EFalse);
    
    if (iSecretEditor) {
        //iSecretEditor->SetFocus(EFalse);
        delete iSecretEditor;
        iSecretEditor = NULL;
    }

    if (iTextEditor) {
        //iTextEditor->SetFocus(EFalse);
        delete iTextEditor;
        iTextEditor = NULL;
    }

    if (iMultTextEditor) {
        //iMultTextEditor->SetFocus(EFalse);
        delete iMultTextEditor;
        iMultTextEditor = NULL;
    }

    iFont = NULL;
}

CFEPControl* CFEPControl::NewL(void)
{
    CFEPControl* self = CFEPControl::NewLC();
    CleanupStack::Pop(self);
    return self;
}

CFEPControl* CFEPControl::NewLC(void)
{
    CFEPControl* self = new (ELeave) CFEPControl;
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

void CFEPControl::ConstructL()
{
    iControlType = CONTROL_NONE;
    iFontColor = TRgb(0, 0, 0);

    CreateWindowL();
    CEikAppUi* appUi = CEikonEnv::Static()->EikAppUi();
    SetMopParent(appUi);
    //Password
    { 
#if FEP_SYS_SECRET
        //        //for CEikSecrectEditor        
//        TInt16 num_letters = MAX_PASSWORD_LEN;
//        TPckgC<TInt16> dummy(num_letters);
//        TResourceReader reader;
//        reader.SetBuffer(&dummy);
//        iSecretEditor = new (ELeave) CEikSecretEditor;
//        iSecretEditor->ConstructFromResourceL(reader);
//        iSecretEditor->SetDefaultInputMode(EAknEditorSecretAlphaInputMode
//            | EAknEditorNumericInputMode);
//        iSecretEditor->SetContainerWindowL(*this);
//        iSecretEditor->SetSkinBackgroundControlContextL(NULL);
//        iSecretEditor->SetBorder(TGulBorder::ENone); //无边框
//        TBool b = iSecretEditor->HasBorder();
//        TGulBorder boder = iSecretEditor->Border();
//        //TMargins m = boder.Margins();
//        iSecretEditor->SetAdjacent(EGulAdjBottom);
//        iSecretEditor->RevealSecretText(EFalse);
        
        iSecretEditor = new (ELeave) CSecretEditor1();
        iSecretEditor->ConstructL();
        iSecretEditor->SetContainerWindowL(*this);

#else
        iSecretEditor = new (ELeave) CSecretEditor;
        iSecretEditor->ConstructL(MAX_PASSWORD_LEN);
        iSecretEditor->SetContainerWindowL(*this);
        iSecretEditor->SetEdwinObserver(this);
#endif
        
        IntiSecretEditor();
    }

    //TextEditor
    {
        iTextEditor = new (ELeave) CEikEdwin;
        iTextEditor->ConstructL(CEikEdwin::ENoWrap, 0, MAX_TEXTEDITOR, 1);
        iTextEditor->SetContainerWindowL(*this);
        iTextEditor->SetEdwinObserver(this);
        InitTextEditor();
    }

    //MultTextEditor
    {
        iMultTextEditor = new (ELeave) CEikEdwin;
        iMultTextEditor->ConstructL(CEikEdwin::ENoAutoSelection, 0, MAX_MULTTEXTEDITOR, 0); //CEikEdwin::ENoAutoSelection
        iMultTextEditor->SetContainerWindowL(*this);
        iMultTextEditor->SetEdwinObserver(this);
        InitMultTextEditor();
    }

    SetRect(TRect(0, 0, 0, 0));
    SetControlFocus(EFalse);
    MakeVisible(EFalse);
    ActivateL();
    iProbe = CResourceManager::GetPtr()->GetProbe();
}

void CFEPControl::SetBgColor(TRgb aColor)
{
    CParaFormat* pf = new (ELeave) CParaFormat();
    CleanupStack::PushL(pf);
    pf->iFillColor = aColor;
    pf->iLineSpacingInTwips = 0;
    TParaFormatMask mask;
    mask.SetAttrib(EAttFillColor);
    mask.SetAttrib(EAttLineSpacing);
           
    switch (iControlType) {
    case CONTROL_PASSWORD:
    {
#if FEP_SYS_SECRET
        iSecretEditor->OverrideColorL(EColorControlBackground, aColor);
#else
        iSecretEditor->SetBackgroundColorL(aColor);
        iSecretEditor->TextView()->SetBackgroundColor(aColor);
        iSecretEditor->SetParaFormatLayer(CParaFormatLayer::NewL(pf, mask));
#endif
        break;
    }
    case CONTROL_EDITBOX:
    {
        iTextEditor->SetBackgroundColorL(aColor);
        iTextEditor->TextView()->SetBackgroundColor(aColor);     
        iTextEditor->SetParaFormatLayer(CParaFormatLayer::NewL(pf, mask));
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SetBackgroundColorL(aColor);
        iMultTextEditor->TextView()->SetBackgroundColor(aColor);
        iMultTextEditor->SetParaFormatLayer(CParaFormatLayer::NewL(pf, mask));
    }
        break;
    default:
        break;
    }
    
    CleanupStack::PopAndDestroy();
}

void CFEPControl::SetControlFocus(TBool aFocus)
{
    SetFocus(aFocus);

    switch (iControlType) {
    case CONTROL_PASSWORD:
    {
        iSecretEditor->SetFocus(aFocus);
    }
        break;
    case CONTROL_EDITBOX:
    {
        iTextEditor->SetFocus(aFocus);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SetFocus(aFocus);
    }
        break;
    default:
        break;
    }
}

void CFEPControl::HandleCommand(TInt aCommand)
{
    TKeyEvent keyEvent;
    keyEvent.iCode = 0;
    keyEvent.iModifiers = 0;
    keyEvent.iRepeats = 0;
    keyEvent.iScanCode = 0;
    TEventCode type = EEventKey;

    switch (aCommand) {
    case EAknSoftkeyOk:
    {
        iSoftKeyDown = ETrue;
        type = EEventKeyDown;
        keyEvent.iScanCode = EStdKeyDevice0;
        //DXEventManager::GetSingleton().OfferKeyEvent(keyEvent, type);
    }
        break;
    case EAknSoftkeyCancel:
    {
        iSoftKeyDown = ETrue;
        type = EEventKeyDown;
        keyEvent.iScanCode = EStdKeyDevice1;
        //DXEventManager::GetSingleton().OfferKeyEvent(keyEvent, type);
    }
        break;
    }
}

TKeyResponse CFEPControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{   
    TBool handleEvent = ETrue;
    
    TInt inputType;
    
    switch (aKeyEvent.iScanCode) {
    case EStdKeyDevice0:
    case EStdKeyDevice1:
    {
        return EKeyWasNotConsumed;
    }
//    case EStdKeyDevice3:
//        return EKeyWasConsumed;
    case EStdKeyEnter:
    {
        if (iControlType == CONTROL_EDITBOX || iControlType == CONTROL_PASSWORD) {
            return EKeyWasConsumed;
        }
        break;
    }
    case EStdKeyUpArrow:
    {
            if (iControlType == CONTROL_MULTEDITBOX) {
                if (!iMultTextEditor->CursorPos()) {
                    handleEvent = EFalse;
                }
            }
            else {
                handleEvent = EFalse;
            }
        //}
        break;
    }
    case EStdKeyDownArrow:
    {
            if (iControlType == CONTROL_MULTEDITBOX) {
                if (iMultTextEditor->CursorPos() >= iMultTextEditor->TextLength()) {
                    handleEvent = EFalse;
                }
            }
            else {
                handleEvent = EFalse;
            }
        //}
        break;
    }
    }
    
    if (!handleEvent)
        return EKeyWasNotConsumed;
    
    switch (iControlType) {
    case CONTROL_PASSWORD:
    {
        iSecretEditor->OfferKeyEventL(aKeyEvent, aType);
        
        if (aType == EEventKeyUp)
        {
            if (iObserver)
                iObserver->HandleSysControlContentChanged();
        }       
        break;
    }
    case CONTROL_EDITBOX:
    {
        iTextEditor->OfferKeyEventL(aKeyEvent, aType);
        
        if (aType == EEventKeyUp && iObserver)
            iObserver->HandleSysControlContentChanged();
        break;
    }
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->OfferKeyEventL(aKeyEvent, aType);
        if (aType == EEventKeyUp && iObserver)
            iObserver->HandleSysControlContentChanged();
        break;
    }
    default:
        return EKeyWasConsumed;
    }
    
    return EKeyWasConsumed;
}

void CFEPControl::SizeChanged(void)
{   
    switch (iControlType) {
    case CONTROL_PASSWORD:
    {
        iSecretEditor->SetExtent(Rect().iTl, Rect().Size());
    }
        break;
    case CONTROL_EDITBOX:
    {
        iTextEditor->SetExtent(Rect().iTl, Rect().Size());
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SetExtent(Rect().iTl, Rect().Size());
        TParaFormatMask mask;
        mask.SetAll();
        CParaFormat* format = CParaFormat::NewL();
        TInt lineSpace = 2;
        TInt spaceInTwip = CEikonEnv::Static()->ScreenDevice()->VerticalPixelsToTwips(lineSpace);
        format->iLineSpacingInTwips = spaceInTwip;
        CParaFormatLayer* formatLayer = CParaFormatLayer::NewL(format, mask);
        iMultTextEditor->SetParaFormatLayer(formatLayer);
        delete format;
        iMultTextEditor->TextLayout()->RestrictScrollToTopsOfLines(EFalse);        
    }
        break;
    case CONTROL_NONE:
    {
//        iSecretEditor->SetExtent(Rect().iTl, Rect().Size());
//        iTextEditor->SetExtent(Rect().iTl, Rect().Size());
//        iMultTextEditor->SetExtent(Rect().iTl, Rect().Size());
    }
        break;
    }
}

TInt CFEPControl::CountComponentControls() const
{
    return iControlType == CONTROL_NONE ? 0 : 1;
}

CCoeControl* CFEPControl::ComponentControl(TInt aIndex) const
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
        return iTextEditor;
    case CONTROL_PASSWORD:
        return iSecretEditor;
    case CONTROL_MULTEDITBOX:
        return iMultTextEditor;
    default:
        return NULL;
    }
}

void CFEPControl::Active(void)
{
    if (IsActive())
        DeActive();
    iActive = ETrue;
    //iControlType = aType;
    SetControlFocus(TRUE);
    MakeVisible(ETrue);
    CCoeEnv::Static()->AppUi()->AddToStackL(this);
    ActivateL();
    if (iActiveNBK) {
        iActiveNBK->OnEditStateChange(ETrue);
    }
    
#if FEP_SYS_SECRET
#else
    if (iControlType == CONTROL_PASSWORD)
    {
        iSecretEditor->SetCursorVisible(EFalse);
        iSecretEditor->StartCursorL(100);
    }
#endif
}

void CFEPControl::DeActive()
{
    if (IsActive()) {
        
        CommitTextL();
        if (iActiveNBK) {
            iActiveNBK->OnEditStateChange(EFalse);
        }
        SetControlFocus(FALSE);
        MakeVisible(EFalse);
        CCoeEnv::Static()->AppUi()->RemoveFromStack(this);
        Reset();
#if FEP_SYS_SECRET
#else
        if (iControlType == CONTROL_PASSWORD)
            iSecretEditor->StopCursor();
#endif
        iActive = EFalse;
        iControlType = CONTROL_NONE;
        if (iOwnFont) {
            CEikonEnv::Static()->ScreenDevice()->ReleaseFont(iFont);
            iFont = NULL;
            iOwnFont = EFalse;
        }
    }
}

TBool CFEPControl::IsActive()
{
    return iActive;//(iControlType != CONTROL_NONE);
}

void CFEPControl::SetObserver(MFEPControlObserver* aObserver)
{
    iObserver = aObserver;
}

void CFEPControl::SetEditState(CNBrKernel* aNBK, TBool aEdit)
{
    iActiveNBK = aNBK;
    if (iEditState != aEdit)
    {
        iSrcEditRect.SetRect(0,0,0,0);
    }
    iEditState = aEdit;
}

void CFEPControl::SetSelection(CNBrKernel* aNBK)
{
    NNode* node = (NNode*) iActiveNBK->GetFocus();
    if (node) {
        TInt start, end;
        renderNode_getSelPos((NRenderNode*) (node->render), &start, &end);
        SetSelection(start, end);
    }   
}

void CFEPControl::PasteTextL(CNBrKernel* aNBK, const TDesC& aText)
{
    if (aNBK)
        DoCommitText(aNBK, aText);
}

void CFEPControl::SetRect(const TRect& aRect)
{
    CCoeControl::SetRect(aRect);
}

void CFEPControl::SetFontColor(TRgb aColor)
{
    iFontColor = aColor;

    switch (iControlType) {
    case CONTROL_PASSWORD:
    {
        IntiSecretEditor();
    }
        break;
    case CONTROL_EDITBOX:
    {
        InitTextEditor();
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        InitMultTextEditor();
    }
        break;
    default:
        break;
    }
}

void CFEPControl::SetFont(CFont* aFont, TBool aOwn)
{
    //_ASSERT(aFont != NULL);
    iFont = aFont;
    iOwnFont = aOwn;
    
    switch (iControlType) {
    case CONTROL_PASSWORD:
    {
#if FEP_SYS_SECRET
        iSecretEditor->SetFont(aFont);
#else
        iSecretEditor->SetFont(aFont);
#endif
        IntiSecretEditor();
    }
        break;
    case CONTROL_EDITBOX:
    {
        InitTextEditor();
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        InitMultTextEditor();
    }
        break;
    default:
        break;
    }
}

CFont*& CFEPControl::GetFont()
{
    return iFont;
}

void CFEPControl::InitTextEditor()
{
    TCharFormat cf;
    TCharFormatMask cfm;
    cfm.SetAttrib(EAttColor);
    cfm.SetAttrib(EAttFontTypeface);
    cfm.SetAttrib(EAttFontHeight);
    //cfm.SetAttrib(EAttVerticalAlignment);
    cf.iFontPresentation.iTextColor = iFontColor;
    if (iFont)
    {
        cf.iFontSpec = iFont->FontSpecInTwips();
    }
    else
        cf.iFontSpec.iHeight = 0;

    iTextEditor->SetCharFormatLayer(CCharFormatLayer::NewL(cf, cfm));
    
    if (iFont)
    {
//                CParaFormat* styleFormat = CParaFormat::NewLC();
//                TParaFormatMask styleMask;
//                TParaBorder boder;
//                boder.iThickness = CEikonEnv::Static()->ScreenDevice()->VerticalPixelsToTwips(yOffset);
//                styleFormat->SetParaBorderL(CParaFormat::EParaBorderTop,boder);
//                styleFormat->SetParaBorderL(CParaFormat::EParaBorderBottom,boder);
//                styleMask.SetAttrib(EAttTopBorder);
//                styleMask.SetAttrib(EAttBottomBorder);
//                iTextEditor->SetParaFormatLayer(CParaFormatLayer::NewL(styleFormat,styleMask));
//                CleanupStack::PopAndDestroy(styleFormat);   
    }
}

void CFEPControl::IntiSecretEditor()
{
#if FEP_SYS_SECRET

    //for CEikSecrectEditor
    iSecretEditor->OverrideColorL(EColorControlText, iFontColor);
    if (iFont)
    {
        iSecretEditor->SetFont(iFont);
    }
#else 
    TCharFormat cf;
    TCharFormatMask cfm;
    cfm.SetAttrib(EAttColor);
    cfm.SetAttrib(EAttFontTypeface);
    cfm.SetAttrib(EAttFontHeight);
    cf.iFontPresentation.iTextColor = iFontColor;
    if (iFont)
    cf.iFontSpec = iFont->FontSpecInTwips();
    else
    cf.iFontSpec.iHeight = 0;

    iSecretEditor->SetCharFormatLayer(CCharFormatLayer::NewL(cf, cfm));
#endif
}

void CFEPControl::InitMultTextEditor()
{
    TCharFormat cf;
    TCharFormatMask cfm;
    cfm.SetAttrib(EAttColor);
    cfm.SetAttrib(EAttFontTypeface);
    cfm.SetAttrib(EAttFontHeight);
    cf.iFontPresentation.iTextColor = iFontColor;
    if (iFont)
        cf.iFontSpec = iFont->FontSpecInTwips();
    else
        cf.iFontSpec.iHeight = 0;

    iMultTextEditor->SetCharFormatLayer(CCharFormatLayer::NewL(cf, cfm));
}

void CFEPControl::SetText(const TDesC& aText, TBool aInsert)
{
    if (IsEdit()) {
        iInitTextLen = aText.Length();
        TPtrC newTextRefer(aText);
        HBufC* newText = NULL;

        switch (iControlType) {
        case CONTROL_EDITBOX:
        {
            if (TInt len = aText.Length()) {
                newText = aText.Alloc();
                TPtr pbuf = newText->Des();
                for (TInt i = 0; i < len; i++) {
                    if ((pbuf)[i] == 0x2029) {
                        pbuf[i] = 0x0020;
                    }
                }
                newTextRefer.Set(*newText);
            }

            if (aInsert) {
                TInt pos = iTextEditor->CursorPos();
                TCursorSelection sel = iTextEditor->Selection();
                if (sel.Length()) {
                    pos = sel.LowerPos();
                }
                iTextEditor->InsertDeleteCharsL(pos, newTextRefer, sel);
                iTextEditor->SetCursorPosL(pos + newTextRefer.Length(), EFalse);
                ReportAknEdStateEventL(MAknEdStateObserver::EAknCursorPositionChanged);
            }
            else
                iTextEditor->SetTextL(&newTextRefer);
        }
            break;
        case CONTROL_MULTEDITBOX:
        {
            if (aText.Length()) {
                newText = aText.Alloc();
                TPtr pbuf = newText->Des();
                for (TInt i = 0; i < aText.Length(); i++) {
                    if ((pbuf)[i] == 0xd || (pbuf)[i] == 0xa) {
                        pbuf[i] = 0x2029;
                    }
                }
                newTextRefer.Set(*newText);
            }

            if (aInsert) {              
                TInt pos = iMultTextEditor->CursorPos();
                TCursorSelection sel = iMultTextEditor->Selection();
                if (sel.Length()) {
                    pos = sel.LowerPos();
                }
                iMultTextEditor->InsertDeleteCharsL(pos, newTextRefer, sel);
                iMultTextEditor->SetCursorPosL(pos + newTextRefer.Length(), EFalse);
                ReportAknEdStateEventL(MAknEdStateObserver::EAknCursorPositionChanged);
            }
            else
                iMultTextEditor->SetTextL(&newTextRefer);
        }
            break;
        case CONTROL_PASSWORD:
        {
#if FEP_SYS_SECRET
            iSecretEditor->Reset();
            iSecretEditor->SetText(aText);
#else
            iSecretEditor->SetPassword(aText);
#endif
        }
            break;
        default:
        {
            iInitTextLen = 0;
            //iTextEditor->SetTextL(&aText);
        }
            break;
        }

        if (newText)
            delete newText;
    }
}

void CFEPControl::HandleEdwinEventL(CEikEdwin* aEdwin, MEikEdwinObserver::TEdwinEvent aEventType)
{
    if (MEikEdwinObserver::EEventTextUpdate == aEventType) {
        
        if(CONTROL_PASSWORD == iControlType)
        {
#if FEP_SYS_SECRET
#else
            iSecretEditor->SetCursorVisible(EFalse);
            iSecretEditor->HideControlText();
            DrawNow();
            iSecretEditor->StartCursorL(100);
#endif
        }   
        if (iObserver)
            iObserver->HandleSysControlContentChanged();
    }
    else if(MEikEdwinObserver::EEventNavigation == aEventType)
    {
        if(CONTROL_PASSWORD == iControlType)
        {
#if FEP_SYS_SECRET
#else
            iSecretEditor->SetCursorVisible(EFalse);
            iSecretEditor->UpdateCursor();
//            DrawNow();
//            iSecretEditor->StartCursorL(100);
#endif
        }
    }
    if (aEdwin == iMultTextEditor)
        UpdateScrollbar();
}

void CFEPControl::UpdateScrollbar()
{
    TRect rect_ScrollBar = iMultTextEditor->Rect();
    //iMultEditor_MaxLine = iMultTextEditor->TextLayout()->GetLineNumber(iMultTextEditor->TextLayout()->DocumentLength()) + 1;
    iMultEditor_MaxLine = iMultTextEditor->TextLayout()->NumFormattedLines();
    iMultEditor_ShowLine = rect_ScrollBar.Size().iHeight
        / (iMultTextEditor->TextLayout()->FormattedHeightInPixels() / iMultEditor_MaxLine);
    iMultEditor_FirstLine = iMultTextEditor->TextLayout()->FirstLineInBand();

    //DEBUGA("max_line:%d show_line:%d first_line:%d", nMultEditor_MaxLine, nMultEditor_ShowLine, nMultEditor_FirstLine);
}

void CFEPControl::CommitTextL(void)
{
    TPtrC referText(KNullDesC16);
    if (iActiveNBK) {
        TInt len = GetTextLength();
        HBufC* buf = NULL;
        if (len) {
            if (iControlType == CONTROL_PASSWORD) {
                buf = HBufC::NewLC(MAX_PASSWORD_LEN);
            }
            else
                buf = HBufC::NewLC(len);

            buf->Des().Zero();
            TPtr pbuf = buf->Des();
            GetText(pbuf);
            referText.Set(*buf);
        }

        DoCommitText(iActiveNBK,referText);
        if (buf)
            CleanupStack::PopAndDestroy(buf);
    }
}

void CFEPControl::DoCommitText(CNBrKernel* aNBK, const TDesC& aText)
{
    NNode* node = (NNode*) aNBK->GetFocus();
    if (!node || (TAGID_TEXTAREA != node->id && TAGID_INPUT != node->id))
        return;

    TInt len = 0;
    wchr* newText = NULL;
    HBufC* buf = NULL;
    if (len = aText.Length()) {
        buf = aText.AllocL();
        TPtr pbuf = buf->Des();
        newText = (wchr*) pbuf.Ptr();
        for (TInt i = 0; i < len; i++) {
            if ((pbuf)[i] == 0x2029) {
                if (TAGID_INPUT == node->id)
                    pbuf[i] = 0x0020;
                else if (TAGID_TEXTAREA == node->id)
                    pbuf[i] = 0xd;
            }
        }
    }

    renderNode_setEditText16((NRenderNode*) (node->render), newText, len);

    if (buf)
        delete buf;
}

void CFEPControl::Reset(void)
{
    SetText(KNullDesC);
    SetBgColor(TRgb(255,255,255));
    SetFontColor(TRgb(0,0,0));
}

CFEPControl::InputType CFEPControl::GetInputMode(void)
{
    InputType eInputType = INPUTTYPE_NONE;

    TCoeInputCapabilities icap = ((CCoeAppUi*) (CCoeEnv::Static()->AppUi()))->InputCapabilities();

    if (icap.FepAwareTextEditor() == 0)
        return INPUTTYPE_NONE;

    CAknEdwinState* edwinState =
        static_cast<CAknEdwinState*> (icap.FepAwareTextEditor()->Extension1()->State(KNullUid));
    TInt inputMode = edwinState->CurrentInputMode();

    switch (inputMode) {
    case EAknEditorNumericInputMode:
    {
        eInputType = INPUTTYPE_123;
    }
        break;
    case 64://EAknEditorFullWidthKatakanaInputMode:
    {
        eInputType = INPUTTYPE_PINYIN;
    }
        break;
    case 256://EAknEditorHiraganaInputMode:
    {
        eInputType = INPUTTYPE_BIHUA;
    }
        break;
    case EAknEditorTextInputMode:
    {
        TInt id = edwinState->CurrentCase();
        switch (id) {
        case EAknEditorUpperCase:
            eInputType = INPUTTYPE_ABC;
            break;
        case EAknEditorLowerCase:
            eInputType = INPUTTYPE_abc;
            break;
        case EAknEditorTextCase:
            eInputType = INPUTTYPE_Abc;
            break;
        default:
            break;
        }
    }
        break;
    default:
        break;
    }

    return eInputType;
}
    
void CFEPControl::ReportAknEdStateEventL(TInt aDelay)
{
    if (iDelay && iDelay->IsActive())
        iDelay->Cancel();

    delete iDelay;
    iDelay = NULL;

    iDelay = CPeriodic::NewL(CPeriodic::EPriorityStandard);
    iDelay->Start(aDelay, aDelay, TCallBack(OnDelay, this));
}

void CFEPControl::ReportAknEdStateEventL(MAknEdStateObserver::EAknEdwinStateEvent aEventType)
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        TCoeInputCapabilities icap = iTextEditor->InputCapabilities();
        CAknEdwinState* edwinState =
            static_cast<CAknEdwinState*> (icap.FepAwareTextEditor()->Extension1()->State(KNullUid));
        if (edwinState)
            edwinState->ReportAknEdStateEventL(aEventType);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        TCoeInputCapabilities icap = iMultTextEditor->InputCapabilities();
        CAknEdwinState* edwinState =
            static_cast<CAknEdwinState*> (icap.FepAwareTextEditor()->Extension1()->State(KNullUid));
        if (edwinState)
            edwinState->ReportAknEdStateEventL(aEventType);
    }
        break;
    case CONTROL_PASSWORD:
    {
        TCoeInputCapabilities icap = iSecretEditor->InputCapabilities();
        CAknEdwinState* edwinState =
            static_cast<CAknEdwinState*> (icap.FepAwareTextEditor()->Extension1()->State(KNullUid));
        if (edwinState)
            edwinState->ReportAknEdStateEventL(aEventType);
    }
        break;
    default:
        break;
    }
}
TInt CFEPControl::OnDelay(TAny* aSelf)
{
    CFEPControl& self = *((CFEPControl*) aSelf);
    self.ReportAknEdStateEventL(MAknEdStateObserver::EAknActivatePenInputRequest);
    self.iDelay->Cancel();
    return 1;
}

void CFEPControl::GetText(TDes& aText)
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        iTextEditor->GetText(aText);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->GetText(aText);
    }
        break;
    case CONTROL_PASSWORD:
    {
#if FEP_SYS_SECRET
        iSecretEditor->GetText(aText);
#else
        iSecretEditor->GetPassword(aText);
#endif
    }
        break;
    default:
        break;
    }
}

TInt CFEPControl::GetTextLength()
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
        return iTextEditor->TextLength();
    case CONTROL_MULTEDITBOX:
        return iMultTextEditor->TextLength();
    case CONTROL_PASSWORD:
#if FEP_SYS_SECRET
        return iSecretEditor->Buffer().Length();
#else
        return iSecretEditor->TextLength();
#endif
    default:
        return 0;
    }
}

TInt CFEPControl::GetCursor()
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
        return iTextEditor->CursorPos();
    case CONTROL_MULTEDITBOX:
        return iMultTextEditor->CursorPos();
    case CONTROL_PASSWORD:
        return 0;
        //return iSecretEditor->CursorPos();
    default:
        return 0;
    }
}

void CFEPControl::SetCursor(TInt aPos)
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        iTextEditor->SetCursorPosL(aPos, FALSE);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SetCursorPosL(aPos, FALSE);
    }
        break;
    case CONTROL_PASSWORD:
    {
        //Do Nothing

        //iSecretEditor->SetCursorPosL(aPos, FALSE);
    }
        break;
    default:
        break;
    }
}

void CFEPControl::SetSelection(TInt aStartPos, TInt aEndPos)
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        iTextEditor->SetSelectionL(aEndPos, aStartPos);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SetSelectionL(aEndPos,aStartPos);
    }
        break;
    case CONTROL_PASSWORD:
    {
        //Do Nothing

        //iSecretEditor->SelectAllL();
    }
        break;
    default:
        break;
    }
}

void CFEPControl::SelectAllL()
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        TInt length = iTextEditor->TextLength();
        iTextEditor->SetSelectionL(length, 0);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SelectAllL();
    }
        break;
    case CONTROL_PASSWORD:
    {
        //Do Nothing

        //iSecretEditor->SelectAllL();
    }
        break;
    default:
        break;
    }
}

void CFEPControl::SetMaxLength(TInt aMaxLen)
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        iTextEditor->SetMaxLength(aMaxLen);
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->SetMaxLength(aMaxLen);
    }
        break;
    case CONTROL_PASSWORD:
    {
        if (aMaxLen < MAX_PASSWORD_LEN)
            iSecretEditor->SetMaxLength(aMaxLen);
    }
        break;
    default:
        break;
    }
}

TInt CFEPControl::GetFontHeight(ControlType aType, TFontSpec aFontSpec)
{
    TInt height, ascent;
    switch (aType) {
    case CONTROL_EDITBOX:
        iTextEditor->TextLayout()->GetFontHeightAndAscentL(aFontSpec, height, ascent);
        return height;
    case CONTROL_MULTEDITBOX:
        iMultTextEditor->TextLayout()->GetFontHeightAndAscentL(aFontSpec, height, ascent);
        return height;
    case CONTROL_PASSWORD:
        //              return iSecretEditor->iFont->HeightInPixels();
        //iSecretEditor->TextLayout()->GetFontHeightAndAscentL(aFontSpec, height, ascent);
        return height;
    default:
        return 0;
    }
}

TInt CFEPControl::GetLineSpace(ControlType aType)
{
    switch (aType) {
    case CONTROL_EDITBOX:
        return iTextEditor->TextLayout()->MinimumLineDescent();
    case CONTROL_MULTEDITBOX:
        return iMultTextEditor->TextLayout()->MinimumLineDescent();
    case CONTROL_PASSWORD:
        //              return 0;
        //return iSecretEditor->TextLayout()->MinimumLineDescent();
    default:
        return 0;
    }
}

void CFEPControl::SetInpuMode(TInputMode aMode)
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        if (aMode == EText) {
            iTextEditor->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
            iTextEditor->SetAknEditorCurrentInputMode(64);
        }
        else if (aMode == ENumber) {
            iTextEditor->SetAknEditorAllowedInputModes(EAknEditorNumericInputMode);
            iTextEditor->SetAknEditorCurrentInputMode(EAknEditorNumericInputMode);
        }
        else if (aMode == EUrl) {
            iTextEditor->SetAknEditorCurrentCase(EAknEditorLowerCase);
            iTextEditor->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
            iTextEditor->SetAknEditorCurrentInputMode(EAknEditorTextInputMode);
        }
    }
        break;
    case CONTROL_MULTEDITBOX:
    {
        if (aMode == EText) {
            iMultTextEditor->SetAknEditorAllowedInputModes(EAknEditorAllInputModes);
            iMultTextEditor->SetAknEditorCurrentInputMode(66);
        }
        else if (aMode == ENumber) {
            iMultTextEditor->SetAknEditorAllowedInputModes(EAknEditorNumericInputMode);
            iMultTextEditor->SetAknEditorCurrentInputMode(EAknEditorNumericInputMode);
        }
    }
        break;
    case CONTROL_PASSWORD:
    {
    }
        break;
    default:
        break;
    }
}

void CFEPControl::HandleTextChangedL()
{
    switch (iControlType) {
    case CONTROL_EDITBOX:
    {
        iTextEditor->HandleTextChangedL();
    }
        break;

    case CONTROL_MULTEDITBOX:
    {
        iMultTextEditor->HandleTextChangedL();
    }
        break;

    case CONTROL_PASSWORD:
    {
#if FEP_SYS_SECRET
#else
        iSecretEditor->HandleTextChangedL();
#endif
    }
        break;
    default:
        break;
    }
}

