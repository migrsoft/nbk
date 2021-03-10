/*
 * nbk_timer.h
 *
 *  Created on: 2011-2-11
 *      Author: wuyulun
 */

#ifndef NBK_TIMER_H_
#define NBK_TIMER_H_

#include "config.h"
    
#ifdef __cplusplus
extern "C" {
#endif

// 定时器接口
// 时间单位：毫秒 1/1000s

typedef void(*TIMER_CALLBACK)(void* user);

typedef struct _NTimer {
    
    int             id; // timer id
    TIMER_CALLBACK  func;
    void*           user;

	void*			pfd;
    
    nbool            run : 1;
    
} NTimer, NBK_Timer;

NTimer* tim_create(TIMER_CALLBACK cb, void* user, void* pfd);
void tim_delete(NTimer** timer);
void tim_start(NTimer* timer, int delay, int interval);
void tim_stop(NTimer* timer);
void tim_setCallback(NTimer* timer, TIMER_CALLBACK cb, void* user);

// implemented by platform
void NBK_timerCreate(void* pfd, NTimer* timer);
void NBK_timerDelete(void* pfd, NTimer* timer);
void NBK_timerStart(void* pfd, NTimer* timer, int delay, int interval);
void NBK_timerStop(void* pfd, NTimer* timer);

void NBK_callLater(void* pfd, int pageId, TIMER_CALLBACK cb, void* user);

#ifdef __cplusplus
}
#endif

#endif /* NBK_TIMER_H_ */
