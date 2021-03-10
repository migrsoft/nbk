/*
 ============================================================================
 Name       : DecodeThread.h
 Author     : Laikesheng
 Version    : 1.0
 Description: Image decode thread
 ============================================================================
 */

#ifndef DECODETHREAD_H_
#define DECODETHREAD_H_

// INCLUDES
#include <e32base.h>

class CImageImplFrame;
class CDecoder;

class CDecodeThread: public CBase
{
public:
    // Constructors and destructor

    static CDecodeThread* NewL();
    virtual ~CDecodeThread();

    void Decode(TRequestStatus* aStatus, const TDesC8& aData,
        RPointerArray<CImageImplFrame>& aDestination, TDisplayMode aDisplayMode);
    
private:
    // Private constructors

    CDecodeThread();
    void ConstructL();
    static TInt ScaleInThread(TAny *aPtr);

private:
    // Data
    // Image status & state
    RThread iDecoderThread;
    static CDecoder* iDecoder;
};

#endif   // DECODETHREAD_H

