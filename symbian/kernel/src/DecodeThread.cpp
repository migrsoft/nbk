#include <e32std.h>
#include <EIKENV.H>
#include <ImageConversion.h>
#include "DecodeThread.h"
#include "ImageImpl.h"

class CBmElem: public CBase
{
public:
    CBmElem(RPointerArray<CImageImplFrame>& aDestination);
    ~CBmElem();
    void ConstructL();

    TPtrC8 iData;
    TThreadId iParentThreadId;
    TRequestStatus* iRequestStatus;
    TDisplayMode iDisplayMode;
    RPointerArray<CImageImplFrame>& iFrames;//not owned
};

enum DecoderState
{
    ENewDecodeRequest, EDecodeInProgress, EDecoderIdle, EDecoderTerminate
};

class CDecoder: public CActive
{
public:
    // Constructors and destructor
    static CDecoder* NewL(/*RPointerArray<CImageImplFrame>& aDestination*/);
    virtual ~CDecoder();

public:
    void Open(TRequestStatus* aStatus, const TDesC8& aData,
        RPointerArray<CImageImplFrame>& aDestination, TDisplayMode aDisplayMode);
    
    void Lock()
    {
        iDecoderLock.Wait();
    }
    void Release()
    {
        iDecoderLock.Signal();
    }
    void Terminate()
    {
        iDecodeState = EDecoderTerminate;
    }

    void Reset(void);
    
private:
    // From base class CActive
    void DoCancel();
    void RunL();
    TInt RunError(TInt aError);
    void SignalParent(TInt aError);
    void StartDecodeL();
    TBool ContinueDecode();
    void SetIdle();
    void NextDecode();
    void DuplicateFrame();
private:
    // Private constructors
    CDecoder(/*RPointerArray<CImageImplFrame>& aDestination*/);
    void ConstructL();

private:
    // Data
    RPointerArray<CBmElem> iElems;
    CImageDecoder* iDecoder;
    TInt16 iCurrFrame;
    RPointerArray<CImageImplFrame> iFrames;//owned
    RFastLock iDecoderLock;
    DecoderState iDecodeState;

    friend class CDecodeThread;
};

// FORWARD DECLARATIONS
CDecoder *CDecodeThread::iDecoder = NULL;

CBmElem::CBmElem(RPointerArray<CImageImplFrame>& aDestination) :
    iFrames(aDestination)
{
}

CBmElem::~CBmElem()
{    
}

void CBmElem::ConstructL()
{   
}

CDecoder::CDecoder(/*RPointerArray<CImageImplFrame>& aDestination*/) :
    CActive(CActive::EPriorityHigh),iDecoder(NULL)//, iFrames(aDestination)
{
    CActiveScheduler::Add(this);
}

void CDecoder::ConstructL()
{
    User::LeaveIfError(iDecoderLock.CreateLocal());
    iElems.ReserveL(6);
    SetIdle();
}

CDecoder* CDecoder::NewL(/*RPointerArray<CImageImplFrame>& aDestination*/)
{
    CDecoder* self = new (ELeave) CDecoder();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(); // self
    return self;
}

CDecoder::~CDecoder()
{
    Cancel();
    iElems.ResetAndDestroy();
    Reset();
    iDecoderLock.Close();
}

void CDecoder::Open(TRequestStatus* aStatus, const TDesC8& aData,
    RPointerArray<CImageImplFrame>& aDestination, TDisplayMode aDisplayMode)
{
    // FbsSession is needed for parent thread if it doesn't have already
    if (!RFbsSession::GetSession())
        RFbsSession::Connect();

    CBmElem* elem = new CBmElem(aDestination);
    elem->iRequestStatus = aStatus;
    elem->iData.Set(aData);
    elem->iParentThreadId = RThread().Id();
    elem->iDisplayMode = aDisplayMode;
    iElems.Append(elem);
}

void CDecoder::Reset(void)
{
    if (iDecoder) {
        delete iDecoder;
        iDecoder = NULL;
    }
    iCurrFrame = 0;
    iFrames.ResetAndDestroy();
}

void CDecoder::SetIdle()
{
    iDecodeState = EDecoderIdle;
    //iElem.iParentThreadId = 0;
    if (!IsActive()) {
        iStatus = KRequestPending;
        SetActive();
    }
}

void CDecoder::NextDecode()
{
    if (IsActive())
        Cancel();
    
    SetActive();
    iStatus = KRequestPending;
    iDecodeState = ENewDecodeRequest;
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrNone);
}

void CDecoder::StartDecodeL()
{
    Reset();
    CBmElem& elem = *iElems[0];
    iDecoder = CImageDecoder::DataNewL(CEikonEnv::Static()->FsSession(), elem.iData);
    TInt nCnt = iDecoder->FrameCount();
    TDisplayMode maskmode;
    
    for (int i = 0; i < nCnt; i++) {

        TFrameInfo frameInfo = iDecoder->FrameInfo(i);

        CImageImplFrame* fr = new (ELeave) CImageImplFrame;
        fr->iFrame = new (ELeave) CFbsBitmap();
        //fr->iFrame->Create(frameInfo.iOverallSizeInPixels, elem.iDisplayMode);
        fr->iFrame->Duplicate(elem.iFrames[i]->iFrame->Handle());       
        if (elem.iFrames[i]->iMask) {
            fr->iMask = new (ELeave) CFbsBitmap;
            fr->iMask->Duplicate(elem.iFrames[i]->iMask->Handle());
        }
        iFrames.Append(fr);
    }
    
    // start decoding
    iCurrFrame = 0;
    ContinueDecode();
}

TBool CDecoder::ContinueDecode()
{
    TInt nCnt = iDecoder->FrameCount();
    if (iCurrFrame < nCnt) {
        if (iFrames[iCurrFrame]->iMask)
            iDecoder->Convert(&iStatus, *(iFrames[iCurrFrame]->iFrame),
                *(iFrames[iCurrFrame]->iMask), iCurrFrame);
        else
            iDecoder->Convert(&iStatus, *(iFrames[iCurrFrame]->iFrame), iCurrFrame);
        
        SetActive();
        iDecodeState = EDecodeInProgress;
        iCurrFrame++;
        return ETrue;
    }
    return EFalse;
}

void CDecoder::DoCancel()
{
    iDecoder->Cancel();
    SignalParent(KErrCancel);
    SetIdle();
}

void CDecoder::RunL()
{
    Lock();
    switch (iDecodeState) {
    case ENewDecodeRequest:
        StartDecodeL(); // start async decode
        break;
    case EDecodeInProgress:
        if (iStatus.Int() == KErrNone && ContinueDecode())
            break;
        //DuplicateFrame();
        SignalParent(iStatus.Int());

        CBmElem* tmp = iElems[0];
        delete tmp;
        iElems.Remove(0);
        Reset();
        
        if (iElems.Count())
            NextDecode();
        else
            SetIdle();
        break;
    case EDecoderTerminate:
        // if any thread is waiting for decode, signal it
        for (TInt i = 0; i < iElems.Count(); i++) {
            CBmElem& elem = *iElems[i];
            if (elem.iParentThreadId)
                SignalParent(KErrCompletion);
        }
        CActiveScheduler::Stop();
        break;
    default:
        SetIdle();
    }
    Release();
}

TInt CDecoder::RunError(TInt aError)
{
    SignalParent(aError);
    SetIdle();
    return KErrNone;
}

void CDecoder::SignalParent(TInt aError)
{
    RThread parent;
    CBmElem& elem = *iElems[0];
    parent.Open(elem.iParentThreadId);
    parent.RequestComplete(elem.iRequestStatus, aError);
    parent.Close();
}

void CDecoder::DuplicateFrame()
{
    TInt cnt = iFrames.Count();
    for (TInt i = 0; i < cnt; i++) {
        CImageImplFrame* frame = iFrames[i];

        CBmElem& elem = *iElems[0];
        elem.iFrames[i]->iFrame->Duplicate(frame->iFrame->Handle());

        if (frame->iMask) {
            elem.iFrames[i]->iMask->Duplicate(frame->iMask->Handle());
        }
    }
}

CDecodeThread* CDecodeThread::NewL()
{
    CDecodeThread* self = new (ELeave) CDecodeThread();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
}

CDecodeThread::CDecodeThread()
{
}

CDecodeThread::~CDecodeThread()
{
    // signal the thread to do cleanup and stop
    iDecoder->Terminate();
    TRequestStatus *ps = &(iDecoder->iStatus);
    iDecoderThread.RequestComplete(ps, KErrNone);
    iDecoderThread.Close();
}

void CDecodeThread::ConstructL()
{
    _LIT(KThreadName, "ImageDecoder");
    //RAllocator &allocator = User::Allocator();
    TInt err = iDecoderThread.Create(KThreadName, CDecodeThread::ScaleInThread,
        KDefaultStackSize, NULL, this);
    if (err == KErrAlreadyExists) {
        // It may happen only in rare cases, if container browserengine..dll was unloaded and loaded again before
        // syncDecodeThread could close itself (scheduled). In that case, we kill that thread since we can't reuse that
        // and create a new one with same name.
        RThread th;
        th.Open(KThreadName);
        th.Kill(KErrNone);
        th.Close();

        // Leave this time if problem still exists
        User::LeaveIfError(iDecoderThread.Create(KThreadName, CDecodeThread::ScaleInThread,
            KDefaultStackSize, NULL, this));
    }

    iDecoderThread.SetPriority(EPriorityMore);
    TRequestStatus status = KRequestPending;
    iDecoderThread.Rendezvous(status);
    iDecoderThread.Resume();
    User::WaitForRequest(status);
    User::LeaveIfError(status.Int());
}

void CDecodeThread::Decode(TRequestStatus* aStatus, const TDesC8& aData, RPointerArray<
    CImageImplFrame>& aDestination, TDisplayMode aDisplayMode)
{
    iDecoder->Lock();
    //notify decoder thread about new request
    //TRequestStatus status = KRequestPending;
    *aStatus = KRequestPending;
    iDecoder->Open(aStatus, aData, aDestination, aDisplayMode);

    if (EDecoderIdle == iDecoder->iDecodeState) {
        iDecoder->iDecodeState = ENewDecodeRequest;
        TRequestStatus *ps = &(iDecoder->iStatus);
        iDecoderThread.RequestComplete(ps, KErrNone);
    }
    //wait for decode complete
    //User::WaitForRequest(status);
    iDecoder->Release();
}

TInt CDecodeThread::ScaleInThread(TAny *aPtr)
{
    CTrapCleanup* cleanup = CTrapCleanup::New();
    CActiveScheduler* as = new CActiveScheduler;
    CActiveScheduler::Install(as);
    RFbsSession fbs;
    fbs.Connect();
    
    // start event loop
    TRAPD(err, iDecoder = CDecoder::NewL());
    if (err == KErrNone) {
        // wait for decode request from client threads
        RThread().Rendezvous(KErrNone);
        CActiveScheduler::Start();
    }
    else {
        RThread().Rendezvous(err);
    }

    // thread shutdown 
    delete as;
    delete iDecoder;
    delete cleanup;
    fbs.Disconnect();
    REComSession::FinalClose();
    return KErrNone;
}
