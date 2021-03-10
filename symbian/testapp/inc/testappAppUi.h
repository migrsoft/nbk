/*
 ============================================================================
 Name		: testappAppUi.h
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Declares UI class for application.
 ============================================================================
 */

#ifndef __TESTAPPAPPUI_h__
#define __TESTAPPAPPUI_h__

// INCLUDES
#include <aknappui.h>
#include <CDBLEN.H>
#include "netTypeDef.h"

typedef CArrayFixFlat<TEApList> EapList;

// FORWARD DECLARATIONS
class CtestappAppView;

class CtestappAppUi: public CAknAppUi
{
public:
    void ConstructL();
    CtestappAppUi();
    virtual ~CtestappAppUi();
    
public:
    TBool GetIapL(TEApList& aIapRecord);
    void GetIapList(EapList* pList);
    void OpenIapTable(EapList* pList, const TDesC& name);
    
private:
    void HandleCommandL(TInt aCommand);
    void HandleStatusPaneSizeChange();

private:
    CtestappAppView* iAppView;

    void LoadSample(const TDesC& aFileName, const TDesC8& aUrl);
};

#endif // __TESTAPPAPPUI_h__
