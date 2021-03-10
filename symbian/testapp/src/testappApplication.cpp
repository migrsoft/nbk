/*
 ============================================================================
 Name		: testappApplication.cpp
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Main application class
 ============================================================================
 */

// INCLUDE FILES
#include "testapp.hrh"
#include "testappDocument.h"
#include "testappApplication.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CtestappApplication::CreateDocumentL()
// Creates CApaDocument object
// -----------------------------------------------------------------------------
//
CApaDocument* CtestappApplication::CreateDocumentL()
{
    // Create an testapp document, and return a pointer to it
    return CtestappDocument::NewL(*this);
}

// -----------------------------------------------------------------------------
// CtestappApplication::AppDllUid()
// Returns application UID
// -----------------------------------------------------------------------------
//
TUid CtestappApplication::AppDllUid() const
{
    // Return the UID for the testapp application
    return KUidtestappApp;
}

// End of File
