/*
 ============================================================================
 Name		: testappAppUi.cpp
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : CtestappAppUi implementation
 ============================================================================
 */
// INCLUDE FILES
#include <avkon.hrh>
#include <aknmessagequerydialog.h>
#include <aknnotewrappers.h>
#include <stringloader.h>
#include <f32file.h>
#include <s32file.h>
//#include <hlplch.h>
#include <aknlists.h>
#include <COMMDB.H>
#include <e32cons.h>
#include <testapp_0xEA69F97C.rsg>
//#include "testapp_0xEA69F97C.hlp.hrh"
#include "testapp.hrh"
#include "testapp.pan"
#include "testappApplication.h"
#include "testappAppUi.h"
#include "testappAppView.h"
#ifdef USE_MM32
#include <DXMemoryManager.h>
#endif

_LIT(KFileSample1, "c:\\data\\nbk_sample.htm");
_LIT(KFileSample2, "c:\\data\\nbk_test2.htm");
_LIT(KFileSample3, "c:\\data\\nbk_test3.htm");
_LIT8(KHomeUrl, "file://c/data/nbk_home.htm");
_LIT8(KFileUrl1, "file://c/data/nbk_sample.htm");
_LIT8(KFileUrl2, "file://c/data/nbk_test2.htm");
_LIT8(KFileUrl3, "file://c/data/nbk_test3.htm");

_LIT8(KTestUrl, "http://bbs.sanxia.la/forum.php?mod=viewthread&tid=28946&mobile=yes");

// ============================ MEMBER FUNCTIONS ===============================


// -----------------------------------------------------------------------------
// CtestappAppUi::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CtestappAppUi::ConstructL()
{
    // Initialise app UI with standard value.
    BaseConstructL(CAknAppUi::EAknEnableSkin);

    // Create view object
    iAppView = CtestappAppView::NewL(ClientRect());

    AddToStackL(iAppView);
}

// -----------------------------------------------------------------------------
// CtestappAppUi::CtestappAppUi()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CtestappAppUi::CtestappAppUi()
{
    // No implementation required
}

// -----------------------------------------------------------------------------
// CtestappAppUi::~CtestappAppUi()
// Destructor.
// -----------------------------------------------------------------------------
//
CtestappAppUi::~CtestappAppUi()
{
    if (iAppView) {
        RemoveFromStack(iAppView);
        TRAPD(err1,delete iAppView);
        iAppView = NULL;
    }

#ifdef USE_MM32
    DXBrowser::DXMemoryManager::Exit();
#endif
}

// -----------------------------------------------------------------------------
// CtestappAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CtestappAppUi::HandleCommandL(TInt aCommand)
{
    switch (aCommand) {
    case EEikCmdExit:
    case EAknSoftkeyExit:
        Exit();
        break;

    case EAknSoftkeyBack:
        iAppView->Back();
        break;

    case EForward:
        iAppView->Forward();
        break;

    case EHome:
        iAppView->SetDirectWorkMode();
        iAppView->LoadUrl(KHomeUrl);
        break;

    case ENightScheme:
        iAppView->SwitchNightScheme();
        break;

    case ETestAsync:
        iAppView->SetDirectWorkMode();
//        iAppView->LoadUrl(KTestUrl);
        iAppView->LoadUrl(KFileUrl2);
        break;

    case ETestSync:
//        iAppView->SetDirectWorkMode();
//        LoadSample(KFileSample3, KFileUrl3);
        iAppView->CopyTest();
        break;

    case ESwitchMode:
        iAppView->SwitchNbkWorkMode();
        break;

    case ESwitchModeCute:
        iAppView->SwitchNbkWorkModeCute();
        break;

    case ESwitchModeUck:
        iAppView->SwitchNbkWorkModeUck();
        break;

    case EEnableImage:
        iAppView->EnableImage();
        break;

    case EEnableIncMode:
        iAppView->EnableIncMode();
        break;

    case EStop:
        iAppView->Stop();
        break;

    case ERefresh:
        iAppView->Refresh();
        break;

    case EClearFsCache:
        iAppView->ClearFileCache();
        break;
        
    case EMonkey:
        iAppView->MonkeyTest();
        break;
        
    case ELayoutMode:
        iAppView->ChangeLayoutMode();
        break;
        
    case ERememberUri:
        iAppView->RememberUri();
        break;
        
    case EHelp:
        break;
        
    case EAbout:
    {
        CAknMessageQueryDialog* dlg = new (ELeave) CAknMessageQueryDialog();
        dlg->PrepareLC(R_ABOUT_QUERY_DIALOG);
        HBufC* title = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TITLE);
        dlg->QueryHeading()->SetTextL(*title);
        CleanupStack::PopAndDestroy(); //title
        HBufC* msg = iEikonEnv->AllocReadResourceLC(R_ABOUT_DIALOG_TEXT);
        dlg->SetMessageTextL(*msg);
        CleanupStack::PopAndDestroy(); //msg
        dlg->RunLD();
    }
        break;
        
    default:
        Panic(EtestappUi);
        break;
    }
}
// -----------------------------------------------------------------------------
//  Called by the framework when the application status pane
//  size is changed.  Passes the new client rectangle to the
//  AppView
// -----------------------------------------------------------------------------
//
void CtestappAppUi::HandleStatusPaneSizeChange()
{
    iAppView->SetRect(ClientRect());
}

void CtestappAppUi::LoadSample(const TDesC& aFileName, const TDesC8& aUrl)
{
    // load nbk sample from file for layout testing.

    RFile file;
    HBufC8* buffer = NULL;
    if (file.Open(CCoeEnv::Static()->FsSession(), aFileName, EFileRead || EFileStream) == KErrNone) {
        TInt length = 0;
        file.Size(length);
        buffer = HBufC8::NewL(length);
        TPtr8 bufPtr(buffer->Des());
        file.Read(bufPtr);
        file.Close();
        iAppView->LoadData(aUrl, buffer);
        delete buffer;
    }
}

//-----------------------选择接入点------------------------------

TBool CtestappAppUi::GetIapL(TEApList& aIapRecord)
{
    EapList* iEApList = new (ELeave) EapList(4);
    CleanupStack::PushL(iEApList);

    GetIapList(iEApList);

    if (iEApList->Count() <= 0) {
        CleanupStack::PopAndDestroy(iEApList);
        return FALSE;
    }

    CAknSinglePopupMenuStyleListBox* list = new (ELeave) CAknSinglePopupMenuStyleListBox;
    CleanupStack::PushL(list);

    CAknPopupList* popupList = CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_OK_CANCEL,
        AknPopupLayouts::EMenuWindow);
    CleanupStack::PushL(popupList);

    list->ConstructL(popupList, CEikListBox::ELeftDownInViewRect);
    list->CreateScrollBarFrameL(ETrue);
    list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
        CEikScrollBarFrame::EAuto);

    CDesCArrayFlat* items = new (ELeave) CDesCArrayFlat(4);
    CleanupStack::PushL(items);
    for (TInt i = 0; i < iEApList->Count(); i++) {
        items->AppendL((*iEApList)[i].iName);
    }
    CTextListBoxModel* model = list->Model();
    model->SetItemTextArray(items);
    model->SetOwnershipType(ELbmOwnsItemArray);
    CleanupStack::Pop();
    //请选择网络接入点
    _LIT(szPrompt, /*请选择网络接入点*/"\x8BF7\x9009\x62E9\x7F51\x7EDC\x63A5\x5165\x70B9");
    popupList->SetTitleL(szPrompt);
    list->SetListBoxObserver(popupList);
    TInt popupOk;
    TRAPD(Err,popupOk = popupList->ExecuteLD());
    CleanupStack::Pop();

    if (popupOk) {
        TInt index = list->CurrentItemIndex();
        aIapRecord.iId = (*iEApList)[index].iId;
        aIapRecord.iName = (*iEApList)[index].iName;
        aIapRecord.iApn = (*iEApList)[index].iApn;
    }
    CleanupStack::PopAndDestroy();
    CleanupStack::PopAndDestroy(iEApList);

    return popupOk && (!Err) ? TRUE : FALSE;
}

void CtestappAppUi::GetIapList(EapList* pList)
{
    if (pList) {
        TBuf<KCommsDbSvrMaxColumnNameLength> iIapName;
        TInt err = KErrNone;

        CCommsDatabase* iCommsDB = CCommsDatabase::NewL(EDatabaseTypeIAP);
        CleanupStack::PushL(iCommsDB);
        CCommsDbTableView* gprsTable = iCommsDB->OpenIAPTableViewMatchingBearerSetLC(
            ECommDbBearerGPRS | ECommDbBearerWLAN | ECommDbBearerVirtual,
            ECommDbConnectionDirectionOutgoing);

        User::LeaveIfError(gprsTable->GotoFirstRecord());
        TUint32 id;
        TEApList eap;

        do {
            gprsTable->ReadTextL(TPtrC(COMMDB_NAME), iIapName);
            gprsTable->ReadUintL(TPtrC(COMMDB_ID), id);
            eap.iId = id;
            eap.iName.Copy(iIapName);
            pList->AppendL(eap);
            err = gprsTable->GotoNextRecord();
        } while (err == KErrNone);
        CleanupStack::PopAndDestroy(2);

        OpenIapTable(pList, TPtrC(OUTGOING_GPRS));
    }
}

void CtestappAppUi::OpenIapTable(EapList* pList, const TDesC& name)
{
    if (pList) {
        CCommsDatabase *commsDb = CCommsDatabase::NewL(EDatabaseTypeIAP);
        CleanupStack::PushL(commsDb);
        CCommsDbTableView *tableView = commsDb->OpenTableLC(name);
        TInt err = tableView->GotoFirstRecord();
        while (err == KErrNone) {
            TBuf<KCommsDbSvrMaxColumnNameLength> iIapName;
            tableView->ReadTextL(TPtrC(COMMDB_NAME), iIapName);
            for (TInt i = 0; i < pList->Count(); i++) {
                TEApList list = (*pList)[i];
                if (list.iName == iIapName) {
                    TBuf<KCommsDbSvrMaxColumnNameLength> apn;
                    tableView->ReadTextL(TPtrC(GPRS_APN), apn);
                    apn.LowerCase();
                    list.iApn = apn;
                    (*pList)[i] = list;
                }
            }
            err = tableView->GotoNextRecord();
        }
        CleanupStack::PopAndDestroy(2, commsDb); // tableView, commsDb
    }
}

