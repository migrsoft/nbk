/*
 ============================================================================
 Name		: testappApplication.h
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Declares main application class.
 ============================================================================
 */

#ifndef __TESTAPPAPPLICATION_H__
#define __TESTAPPAPPLICATION_H__

// INCLUDES
#include <aknapp.h>
#include "testapp.hrh"

// UID for the application;
// this should correspond to the uid defined in the mmp file
const TUid KUidtestappApp = { _UID3 };

// CLASS DECLARATION

/**
 * CtestappApplication application class.
 * Provides factory to create concrete document object.
 * An instance of CtestappApplication is the application part of the
 * AVKON application framework for the testapp example application.
 */
class CtestappApplication: public CAknApplication
{
public:
    // Functions from base classes

    /**
     * From CApaApplication, AppDllUid.
     * @return Application's UID (KUidtestappApp).
     */
    TUid AppDllUid() const;

protected:
    // Functions from base classes

    /**
     * From CApaApplication, CreateDocumentL.
     * Creates CtestappDocument document object. The returned
     * pointer in not owned by the CtestappApplication object.
     * @return A pointer to the created document object.
     */
    CApaDocument* CreateDocumentL();
};

#endif // __TESTAPPAPPLICATION_H__
// End of File
