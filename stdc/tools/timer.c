/*
 * timer.c
 *
 *  Created on: 2011-2-11
 *      Author: wuyulun
 */

#include "../inc/nbk_timer.h"

NTimer* tim_create(TIMER_CALLBACK cb, void* user, void* pfd)
{
    NTimer* timer = (NTimer*)NBK_malloc0(sizeof(NTimer));
    N_ASSERT(timer);
    timer->func = cb;
    timer->user = user;
	timer->pfd = pfd;
    NBK_timerCreate(pfd, timer);
    return timer;
}

void tim_delete(NTimer** timer)
{
    NTimer* t = *timer;
	NBK_timerDelete(t->pfd, t);
    NBK_free(t);
    *timer = N_NULL;
}

void tim_start(NTimer* timer, int delay, int interval)
{
	NBK_timerStart(timer->pfd, timer, delay, interval);
    timer->run = 1;
}

void tim_stop(NTimer* timer)
{
    timer->run = 0;
	NBK_timerStop(timer->pfd, timer);
}

void tim_setCallback(NTimer* timer, TIMER_CALLBACK cb, void* user)
{
    timer->func = cb;
    timer->user = user;
}
