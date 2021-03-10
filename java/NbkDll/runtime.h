#pragma once

#ifdef __WIN32__
	#include "win32/jni.h"
#endif
#ifdef __ANDROID__
	#include <jni.h>
#endif
#include "../../stdc/inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

void memory_info_init();
void memory_info();

#ifdef __cplusplus
}
#endif

class TimerMgr {
	
private:
	jweak		OBJ_TimerMgr;
	jmethodID	MID_start;
	jmethodID	MID_stop;

private:
	static const int MAX_TIMERS = 256;

    int     mAllocStart;
	NTimer* mTimers[MAX_TIMERS];

public:
	TimerMgr();
	~TimerMgr();

	void RegisterJavaMethods(JNIEnv* env, jobject obj);

	void Create(NTimer* timer);
	void Delete(NTimer* timer);
	void Start(NTimer* timer, int delay, int interval);
	void Stop(NTimer* timer);

	void OnTimer(int id);
};
