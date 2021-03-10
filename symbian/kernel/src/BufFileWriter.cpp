/*
 ============================================================================
 Name		: BufFileWriter.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CBufFileWriter implementation
 ============================================================================
 */

#include "BufFileWriter.h"
#include <coemain.h>
#include <s32file.h>

#define USE_BUFFER  1

#define WRITE_BUFFER    65536

CBufFileWriter::CBufFileWriter()
{
    iBuffer = NULL;
    iOpen = EFalse;
    iPos = 0;
}

CBufFileWriter::~CBufFileWriter()
{
    Close();
    
    if (iBuffer) {
        NBK_free(iBuffer);
        iBuffer = NULL;
    }
    
    if (iCacheData) {
        iCacheData->ResetAndDestroy();
        delete iCacheData;
        iCacheData = NULL;
    }
    
    if (iName) {
        delete iName;
        iName = NULL;
    }
}

CBufFileWriter* CBufFileWriter::NewLC()
{
    CBufFileWriter* self = new (ELeave) CBufFileWriter();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CBufFileWriter* CBufFileWriter::NewL()
{
    CBufFileWriter* self = CBufFileWriter::NewLC();
    CleanupStack::Pop(); // self;
    return self;
}

void CBufFileWriter::ConstructL()
{
#if USE_BUFFER
    iBuffer = (uint8*)NBK_malloc(WRITE_BUFFER);
#endif
}

void CBufFileWriter::Open(const TDesC& aFileName)
{
    Close();

#if USE_BUFFER
    RFs& rfs = CCoeEnv::Static()->FsSession();
    rfs.MkDirAll(aFileName);
    if (iFile.Replace(rfs, aFileName, EFileWrite) == KErrNone) {
        iOpen = ETrue;
        iPos = 0;
    }
#else
    iName = aFileName.AllocL();
#endif
}

void CBufFileWriter::Write(const TPtrC8 aData)
{
#if USE_BUFFER
    if (iOpen) {
        if (iPos + aData.Length() >= WRITE_BUFFER) {
            TPtrC8 dataPtr(iBuffer, iPos);
            iFile.Write(dataPtr);
            iPos = 0;
        }
        
        NBK_memcpy(iBuffer + iPos, (uint8*)aData.Ptr(), aData.Length());
        iPos += aData.Length();
    }
#else
    if (iCacheData == NULL)
        iCacheData = new (ELeave) CArrayPtrFlat<HBufC8>(30);
    
    iCacheData->AppendL(aData.AllocL());
#endif
}

void CBufFileWriter::Close()
{
#if USE_BUFFER
    if (iOpen) {
        if (iPos) {
            TPtrC8 dataPtr(iBuffer, iPos);
            iFile.Write(dataPtr);
            iPos = 0;
        }
        iFile.Close();
        iOpen = EFalse;
    }
#else
    if (iName) {
        RFs& rfs = CCoeEnv::Static()->FsSession();
        RFileWriteStream writerStream;
        writerStream.PushL();
        User::LeaveIfError(writerStream.Replace(rfs, iName->Des(), EFileWrite));
        for (int i=0; i < iCacheData->Count(); i++) {
            writerStream.WriteL(iCacheData->At(i)->Des());
        }
        writerStream.CommitL();
        writerStream.Pop();
        writerStream.Release();
        
        delete iName;
        iName = NULL;
    }
#endif
}
