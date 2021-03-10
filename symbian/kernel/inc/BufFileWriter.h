/*
 ============================================================================
 Name		: BufFileWriter.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CBufFileWriter declaration
 ============================================================================
 */

#ifndef BUFFILEWRITER_H
#define BUFFILEWRITER_H

// INCLUDES
#include "../../../stdc/inc/config.h"
#include <e32std.h>
#include <e32base.h>
#include <f32file.h>

class CBufFileWriter : public CBase
{
public:
    ~CBufFileWriter();
    static CBufFileWriter* NewL();
    static CBufFileWriter* NewLC();
    
    void Open(const TDesC& aFileName);
    void Write(const TPtrC8 aData);
    void Close();

private:

    CBufFileWriter();
    void ConstructL();
    
private:
    
    uint8*  iBuffer;
    int     iPos;
    
    RFile   iFile;
    bool    iOpen;
    
    CArrayPtrFlat<HBufC8>*  iCacheData;
    HBufC*  iName;
};

#endif // BUFFILEWRITER_H
