/*
 ============================================================================
 Name		: TimerManager.h
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CTimerManager declaration
 ============================================================================
 */

#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <e32std.h>
#include <e32base.h>
#include "nbk_timer.h"
#include "../../stdc/tools/slist.h"

class CNBrKernel;

class CNbkTimer
{
public:
    
    CNbkTimer(NBK_Timer* t);
    virtual ~CNbkTimer();
    
    static TInt OnTimerEvent(TAny* ptr);

    NBK_Timer*  d;
    CPeriodic*  iTimer;
};

/**
 *  CTimerManager
 * 
 */
class CTimerManager: public CBase
{
public:
    ~CTimerManager();

    static CTimerManager* NewL();
    static CTimerManager* NewLC();
    
    static void Destroy();
        
    void AddTimer(CNbkTimer* timer);
    void RemoveTimer(NBK_Timer* timer);
    CNbkTimer* FindTimer(NBK_Timer* timer);
    
    void SetNbk(CNBrKernel* nbk) { iNbk = nbk; }
    void CallLater(int pageId, void* func, void* user);

private:

    CTimerManager();
    void ConstructL();
    
private:
    
    RPointerArray<CNbkTimer>    iTimerList;
    
private:
    
    typedef struct _TCall {
        int     pageId;
        void*   func;
        void*   user;
    } TCall;
    
    NSList* iCallList;
    
    CPeriodic* iCallTimer;
    
    CNBrKernel* iNbk;
    
    static TInt OnCallEvent(TAny* ptr);
    void CallFunc();
};

#endif // TIMERMANAGER_H
