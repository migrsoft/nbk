#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __WIN32__
	#include <locale.h>
#endif
#include "../../stdc/inc/nbk_glue.h"
#include "../../stdc/inc/nbk_timer.h"
#include "../../stdc/tools/str.h"
#include "runtime.h"
#include "NbkDll.h"
#include "com_migrsoft_nbk_TimerMgr.h"
#include "com_migrsoft_nbk_NbkCore.h"

////////////////////////////////////////////////////////////////////////////////
//
// 内存相关
//

static int l_mem_alloc = 0;
static int l_mem_free = 0;

void memory_info_init()
{
	l_mem_alloc = 0;
	l_mem_free = 0;
}

void memory_info()
{
	NBK_LOG("memory allocated %d, freed %d\n", l_mem_alloc, l_mem_free);
}

void* NBK_malloc(size_t size)
{
	l_mem_alloc++;
	return malloc(size);
}

void* NBK_malloc0(size_t size)
{
	l_mem_alloc++;
	return calloc(size, 1);
}

void* NBK_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}

void NBK_free(void* p)
{
	l_mem_free++;
	free(p);
}

void NBK_memcpy(void* dst, void* src, size_t size)
{
	memcpy(dst, src, size);
}

void NBK_memset(void* dst, int8 value, size_t size)
{
	memset(dst, value, size);
}

void NBK_memmove(void* dst, void* src, size_t size)
{
	memmove(dst, src, size);
}

uint32 NBK_currentMilliSeconds(void)
{
	return (uint32)NbkCore::GetInstance()->GetCurrentMillis();
}

uint32 NBK_currentSeconds(void)
{
	return (uint32)(NbkCore::GetInstance()->GetCurrentMillis() / 1000);
}

uint32 NBK_currentFreeMemory(void) // total free memory in bytes
{
	return 5000000;
}

////////////////////////////////////////////////////////////////////////////////
//
// 输入法相关
//

void NBK_fep_enable(void* pfd)
{
}

void NBK_fep_disable(void* pfd)
{
}

void NBK_fep_updateCursor(void* pfd)
{
}

#ifdef __WIN32__
int NBK_conv_gbk2unicode(const char* mbs, int mbLen, wchr* wcs, int wcsLen)
{
	int count = 0;
	_locale_t loc = _create_locale(LC_ALL, "Chinese");
	if (loc) {
		char* m = (char*)NBK_malloc(mbLen + 1);
		nbk_strncpy(m, mbs, mbLen);
		//count = _mbstowcs_l(NULL, m, mbLen, loc);
		count = _mbstowcs_l((wchar_t*)wcs, m, mbLen, loc);
		NBK_free(m);
		__free_locale(loc);
	}
	return count;
}
#endif

#ifdef __ANDROID__
int NBK_conv_gbk2unicode(const char* mbs, int mbLen, wchr* wcs, int wcsLen)
{
	return 0;
}
#endif

nbool NBK_ext_isDisplayDocument(void* document)
{
	return N_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// NBK 定时器调用
//

void NBK_timerCreate(void* pfd, NTimer* timer)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Create(timer);
}

void NBK_timerDelete(void* pfd, NTimer* timer)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Delete(timer);
}

void NBK_timerStart(void* pfd, NTimer* timer, int delay, int interval)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Start(timer, delay, interval);
}

void NBK_timerStop(void* pfd, NTimer* timer)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetTimerMgr()->Stop(timer);
}

////////////////////////////////////////////////////////////////////////////////
//
// Java 定时器调用
//

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_TimerMgr_register
  (JNIEnv * env, jobject obj)
{
	TimerMgr* mgr = NbkCore::GetInstance()->GetTimerMgr();
	mgr->RegisterJavaMethods(env, obj);
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_NbkCore_nbkOnTimer
  (JNIEnv *, jobject, jint id)
{
	TimerMgr* mgr = NbkCore::GetInstance()->GetTimerMgr();
	mgr->OnTimer(id);
}

////////////////////////////////////////////////////////////////////////////////
//
// TimerMgr 接口类
//

TimerMgr::TimerMgr()
{
	OBJ_TimerMgr = NULL;

    mAllocStart = 0;
	NBK_memset(mTimers, 0, sizeof(NTimer*) * MAX_TIMERS);
}

TimerMgr::~TimerMgr()
{
	if (OBJ_TimerMgr) {
		JNIEnv* env = NbkCore::GetInstance()->GetJvm();
		env->DeleteGlobalRef(OBJ_TimerMgr);
		OBJ_TimerMgr = NULL;
	}
}

void TimerMgr::RegisterJavaMethods(JNIEnv* env, jobject obj)
{
	OBJ_TimerMgr = env->NewGlobalRef(obj);

	jclass clazz = env->GetObjectClass(obj);
	MID_start = env->GetMethodID(clazz, "start", "(III)V");
	MID_stop = env->GetMethodID(clazz, "stop", "(I)V");
}

void TimerMgr::Create(NTimer* timer)
{
	timer->id = -1;

    // 从上次分配的下一地址分配定时器，避免同一地址立即被不同定时器使用

	for (int j = 0; j < 2; j++) {
		for (int i = mAllocStart; i < MAX_TIMERS; i++) {
			if (mTimers[i] == NULL) {
				mTimers[i] = timer;
				timer->id = i++;
				mAllocStart = (i == MAX_TIMERS) ? 0 : i;
				return;
			}
		}
		mAllocStart = 0;
	}
}

void TimerMgr::Delete(NTimer* timer)
{
    if (timer->id != -1) {
        mTimers[timer->id] = NULL;
    }
}

void TimerMgr::Start(NTimer* timer, int delay, int interval)
{
    //printf("timer %d start\n", timer->id);
    if (timer->id != -1) {
	    JNIEnv* env = NbkCore::GetInstance()->GetJvm();
	    env->CallVoidMethod(OBJ_TimerMgr, MID_start, timer->id, delay, interval);
    }
}

void TimerMgr::Stop(NTimer* timer)
{
    //printf("timer %d stop\n", timer->id);
    if (timer->id != -1) {
	    JNIEnv* env = NbkCore::GetInstance()->GetJvm();
	    env->CallVoidMethod(OBJ_TimerMgr, MID_stop, timer->id);
    }
}

void TimerMgr::OnTimer(int id)
{
	NTimer* t = mTimers[id];
	if (t->run)
		t->func(t->user);
}

////////////////////////////////////////////////////////////////////////////////
//
// 字符转换
//

int NBK_atoi(const char* s)
{
	return atoi(s);
}

float NBK_atof(const char* s)
{
	return (float)atof(s);
}
