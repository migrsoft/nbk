#ifndef __MONKEY_H__
#define __MONKEY_H__

#include <w32std.h>
#include "NBrKernel.h"

class MonkeyEvent
{
public:

    typedef enum _monkeyEventType
    {
        EType_Key, EType_Mouse
    } EMonkeyEventType;

    MonkeyEvent(EMonkeyEventType type)
    {
        iEventType = type;
    }

    virtual ~MonkeyEvent()
    {
    }

    virtual TBool SimulateEvent(void) = 0;

    int GetEventType()
    {
        return iEventType;
    }

protected:
    EMonkeyEventType iEventType;
};

class MonkeyMouseEvent: public MonkeyEvent, public CBase
{
public:
    MonkeyMouseEvent(TRawEvent::TType aType, const TPoint& aPos);

    TPoint GetPositon(void)
    {
        return iPos;
    }

    TRawEvent::TType GetType()
    {
        return iType;
    }

    virtual TBool SimulateEvent(void);

public:
    TRawEvent::TType iType;
    TPoint iPos;
};

class CMonkeyTester: public CBase
{
public:

    typedef enum _MonkeyOpt
    {
        eWaitLoadFinish, eMonkeyEventInterval
    } eMonkeyOpt;

    CMonkeyTester(CNBrKernel* aNbrKernel,TUid aUid);
    ~CMonkeyTester(void);

    TBool Start(const TDesC8& aUrl, CNBrKernel::eMonkeyType aMode, TInt aEventCnt);
    void Stop(void);
    void SetOpt(eMonkeyOpt aType, TInt aValue);

    TBool IsMonkeyRuning(void)
    {
        return iMonkeyRun;
    }
    CNBrKernel::eMonkeyType GetMode(void)
    {
        return iMode;
    }

    void OnNbkEvent(CNBrKernel::TNbkEvent& aEvent);

private:

    void RunMonkey(void);
    void ResetUrl(void);
    void DoHitPageLinks();
    TInt GetRangNum(TInt startNum, TInt endNum);
    
    //------------    for random    -------------
    static TInt OnTimer(TAny* ptr);

    void InitRandom(void);
    void UninitRandom(void);    
    
    void ResetRandom(void);
    MonkeyEvent* NextEvent();
    void MakeRandomEvent(void);
    void MakeMouseEvent(TBool aMove);
    TUint DoRandomMonkey(void);
    void FindAndShow();
    
    typedef enum _RandomEventType
    {
        ETouch = 0, EMove, EKeyNavigation, EKeyOk, EOtherKey, ELast
    };
    
    CPeriodic* iTimer;
    TUint iRandomEventPercent[ELast];
    RPointerArray<MonkeyEvent> iRandomEventQueue;
    TUint iFailedEventCnt;
    static RWsSession wsSession;
    static TBool iInited;
    
    //------------    for hit only links    -------------
    HBufC8* iFixedMonkeyUrl;
    TBool iFixedMonkeyUrlSeted;
    
    //------------    other    -------------
    CNBrKernel* iNbrKernel;
    CNBrKernel::eMonkeyType iMode;
    TInt iMonkeyEventCnt;
    TInt iUrlLoadCnt;
    TBool iMonkeyRun;
    TUid iUid;
    //options
    TInt iMonkeyEventInterval;//milisecond
    TBool iWaitLoadFinish;
};

#endif
