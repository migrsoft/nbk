/*
 ============================================================================
 Name		: testappDocument.h
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Declares document class for application.
 ============================================================================
 */

#ifndef __TESTAPPDOCUMENT_h__
#define __TESTAPPDOCUMENT_h__

// INCLUDES
#include <akndoc.h>

// FORWARD DECLARATIONS
class CtestappAppUi;
class CEikApplication;

// CLASS DECLARATION

/**
 * CtestappDocument application class.
 * An instance of class CtestappDocument is the Document part of the
 * AVKON application framework for the testapp example application.
 */
class CtestappDocument: public CAknDocument
{
public:
    // Constructors and destructor

    /**
     * NewL.
     * Two-phased constructor.
     * Construct a CtestappDocument for the AVKON application aApp
     * using two phase construction, and return a pointer
     * to the created object.
     * @param aApp Application creating this document.
     * @return A pointer to the created instance of CtestappDocument.
     */
    static CtestappDocument* NewL(CEikApplication& aApp);

    /**
     * NewLC.
     * Two-phased constructor.
     * Construct a CtestappDocument for the AVKON application aApp
     * using two phase construction, and return a pointer
     * to the created object.
     * @param aApp Application creating this document.
     * @return A pointer to the created instance of CtestappDocument.
     */
    static CtestappDocument* NewLC(CEikApplication& aApp);

    /**
     * ~CtestappDocument
     * Virtual Destructor.
     */
    virtual ~CtestappDocument();

public:
    // Functions from base classes

    /**
     * CreateAppUiL
     * From CEikDocument, CreateAppUiL.
     * Create a CtestappAppUi object and return a pointer to it.
     * The object returned is owned by the Uikon framework.
     * @return Pointer to created instance of AppUi.
     */
    CEikAppUi* CreateAppUiL();

private:
    // Constructors

    /**
     * ConstructL
     * 2nd phase constructor.
     */
    void ConstructL();

    /**
     * CtestappDocument.
     * C++ default constructor.
     * @param aApp Application creating this document.
     */
    CtestappDocument(CEikApplication& aApp);

};

#endif // __TESTAPPDOCUMENT_h__
// End of File
