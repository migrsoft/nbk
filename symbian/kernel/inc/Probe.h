/*
 ============================================================================
 Name		: Probe.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CProbe declaration
 ============================================================================
 */

#ifndef PROBE_H
#define PROBE_H

// INCLUDES
#include <e32std.h>
#include <e32base.h>

// CLASS DECLARATION

/**
 *  CProbe
 * 
 */

class CProbe
{
public:

    CProbe();
    ~CProbe();
    
    static void FileInit(const TDesC& aFileName, TBool aCreate = EFalse);
    static void FileWrite(const TDesC& aFileName, const TDesC8& aData);
    
    void OutputUint(unsigned int i);
    void OutputInt(int i);
    void OutputWchr(unsigned short* wc, int length);
    void OutputChar(char* mb, int length);
    void OutputTRect(TRect aRect);
    void OutputTPoint(TPoint aPoint);
    void OutputReturn();
    
    void OutputTime();
    
    void Output(void* dbgInfo);
    void Output(const TDesC& aText);
    
    void Flush();
    
private:
    
    TBool   iBuffered;
    char*   iBuffer;
    int     iBufPos;
    
    TBool   iOutputTab; // 是否输出 \t
};

#endif // PROBE_H
