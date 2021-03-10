#ifndef __NBK_RUNTIME_H__
#define __NBK_RUNTIME_H__

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_timer.h"
#include <SDL.h>

#define MAX_TIMERS	256

#ifdef __cplusplus
extern "C" {
#endif

extern int nbk_key_map[];

void runtime_init(void);
void runtime_end(void);

//nbool runtime_processTimers(void);

void sync_wait(const char* caller);
void sync_post(const char* caller);

typedef struct _NBK_timerImpl {
	NTimer*	t;
	int		delay;
	int		interval;
	int		elapse;
	Uint32	start;
	nbool	active; // 使用中
    nbool    ready; // 可触发
} NBK_timerImpl;

typedef struct _NBK_timerMgr {
	NBK_timerImpl	lst[MAX_TIMERS];
	//SDL_TimerID		id;
    nbool            active;
} NBK_timerMgr;

NBK_timerMgr* timerMgr_create(void);
void timerMgr_delete(NBK_timerMgr** timerMgr);

void timerMgr_createTimer(NBK_timerMgr* timerMgr, NTimer* timer);
void timerMgr_removeTimer(NBK_timerMgr* timerMgr, NTimer* timer);
void timerMgr_startTimer(NBK_timerMgr* timerMgr, NTimer* timer, int delay, int interval);
void timerMgr_stopTimer(NBK_timerMgr* timerMgr, NTimer* timer);

void timerMgr_run(NBK_timerMgr* timerMgr);

// 平台相关函数

nbool nbk_pathExist(const char* path);
nbool nbk_makeDir(const char* path);
void nbk_removeDir(const char* path);
void nbk_removeMultiDir(const char* path, const char* match);
void nbk_removeFile(const char* fname);

#ifdef __cplusplus
}
#endif

#endif
