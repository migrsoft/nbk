/*
 ============================================================================
 Name		: NBrKernel.pan
 Author	  : wuyulun
 Copyright   : 2010,2011 (C) MIC
 Description : Panic codes
 ============================================================================
 */

#ifndef __NBRKERNEL_PAN__
#define __NBRKERNEL_PAN__

//  Data Types

enum TNBrKernelPanic
{
    ENBrKernelNullPointer
};

//  Function Prototypes

GLREF_C void Panic(TNBrKernelPanic aPanic);

#endif  // __NBRKERNEL_PAN__

