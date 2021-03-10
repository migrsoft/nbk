/*
 ============================================================================
 Name		: testappDocument.cpp
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : CtestappDocument implementation
 ============================================================================
 */

// INCLUDE FILES
#include "testappAppUi.h"
#include "testappDocument.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CtestappDocument::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CtestappDocument* CtestappDocument::NewL(CEikApplication& aApp)
{
    CtestappDocument* self = NewLC(aApp);
    CleanupStack::Pop(self);
    return self;
}

// -----------------------------------------------------------------------------
// CtestappDocument::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CtestappDocument* CtestappDocument::NewLC(CEikApplication& aApp)
{
    CtestappDocument* self = new (ELeave) CtestappDocument(aApp);

    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

// -----------------------------------------------------------------------------
// CtestappDocument::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CtestappDocument::ConstructL()
{
    // No implementation required
}

// -----------------------------------------------------------------------------
// CtestappDocument::CtestappDocument()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CtestappDocument::CtestappDocument(CEikApplication& aApp) :
    CAknDocument(aApp)
{
    // No implementation required
}

// ---------------------------------------------------------------------------
// CtestappDocument::~CtestappDocument()
// Destructor.
// ---------------------------------------------------------------------------
//
CtestappDocument::~CtestappDocument()
{
    // No implementation required
}

// ---------------------------------------------------------------------------
// CtestappDocument::CreateAppUiL()
// Constructs CreateAppUi.
// ---------------------------------------------------------------------------
//
CEikAppUi* CtestappDocument::CreateAppUiL()
{
    // Create the application user interface, and return a pointer to it;
    // the framework takes ownership of this object
    return new (ELeave) CtestappAppUi;
}

// End of File
