/*
 ============================================================================
 Name		: testapp.cpp
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : Main application class
 ============================================================================
 */

// INCLUDE FILES
#include <eikstart.h>
#include "testappApplication.h"

LOCAL_C CApaApplication* NewApplication()
{
    return new CtestappApplication;
}

GLDEF_C TInt E32Main()
{
    return EikStart::RunApplication(NewApplication);
}

