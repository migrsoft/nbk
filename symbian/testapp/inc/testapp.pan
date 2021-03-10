/*
 ============================================================================
 Name		: testapp.pan
 Author	  : wuyulun
 Copyright   : Your copyright notice
 Description : This file contains panic codes.
 ============================================================================
 */

#ifndef __TESTAPP_PAN__
#define __TESTAPP_PAN__

/** testapp application panic codes */
enum TtestappPanics
{
    EtestappUi = 1
// add further panics here
};

inline void Panic(TtestappPanics aReason)
{
    _LIT(applicationName, "testapp");
    User::Panic(applicationName, aReason);
}

#endif // __TESTAPP_PAN__
