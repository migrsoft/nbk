// create by wuyulun, 2011.12.24

#include "nbk_conf.h"
#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_timer.h"
#include "../stdc/tools/str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "runtime.h"
#include "nbk.h"
#include "md5.h"
#ifdef PLATFORM_WIN32
    #include <io.h>
    #include <direct.h>
    #include <locale.h>
#endif
#ifdef PLATFORM_LINUX
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <glob.h>
#endif

#define MAX_KEY_MAP     128

static SDL_sem* l_sem = N_NULL; // 用于同步多线程操作内核

static int l_mem_alloc = 0;
static int l_mem_free = 0;
static int l_sync_count = 0;
static int l_max_timers = 0;

int nbk_key_map[MAX_KEY_MAP];

//#define MEM_TRACK

#ifdef MEM_TRACK

#define MEM_TRACK_MAX_SIZE	10000

typedef struct _mem_track {
	void*	ptr;
	size_t	size;
} mem_track;

static mem_track* l_mem_track = N_NULL;
static int l_mem_top = 0;

static void mem_track_add(void* ptr, int size)
{
	int i;

	if (l_mem_track == N_NULL)
		l_mem_track = (mem_track*)calloc(MEM_TRACK_MAX_SIZE, sizeof(mem_track));

	for (i=0; i < MEM_TRACK_MAX_SIZE; i++) {
		if (l_mem_track[i].ptr == N_NULL) {
			l_mem_track[i].ptr = ptr;
			l_mem_track[i].size = size;
			l_mem_top = N_MAX(l_mem_top, i + 1);
			break;
		}
	}
}

static void mem_track_remove(void* ptr)
{
	int i;

	for (i=0; i < MEM_TRACK_MAX_SIZE; i++) {
		if (l_mem_track[i].ptr == ptr) {
			l_mem_track[i].ptr = N_NULL;
			break;
		}
	}
}

static void mem_track_leak(void)
{
	int i;
	mem_track* track;

	for (i=0; i < MEM_TRACK_MAX_SIZE; i++) {
		track = &l_mem_track[i];
		if (track->ptr) {
			fprintf(stderr, "mem track: leak!!! ADDR: %p SIZE: %u\n", track->ptr, track->size);
		}
	}
	fprintf(stderr, "mem track: top %d\n", l_mem_top);

	free(l_mem_track);
	l_mem_track = N_NULL;
}

#endif

static void key_map_init(void)
{
    NBK_memset(nbk_key_map, 0, sizeof(int) * MAX_KEY_MAP);

    nbk_key_map[SDLK_BACKQUOTE] = 0; // 反引号

    nbk_key_map[SDLK_1] = SDLK_EXCLAIM;
    nbk_key_map[SDLK_2] = SDLK_AT;
    nbk_key_map[SDLK_3] = SDLK_HASH;
    nbk_key_map[SDLK_4] = SDLK_DOLLAR;
    nbk_key_map[SDLK_5] = 0;
    nbk_key_map[SDLK_6] = SDLK_CARET;
    nbk_key_map[SDLK_7] = SDLK_AMPERSAND;
    nbk_key_map[SDLK_8] = SDLK_ASTERISK;
    nbk_key_map[SDLK_9] = SDLK_LEFTPAREN;
    nbk_key_map[SDLK_0] = SDLK_RIGHTPAREN;

    nbk_key_map[SDLK_MINUS] = SDLK_UNDERSCORE; // 减
    nbk_key_map[SDLK_EQUALS] = SDLK_PLUS; // 等于

    nbk_key_map[SDLK_LEFTBRACKET] = 0; // 左方括
    nbk_key_map[SDLK_RIGHTBRACKET] = 0;
    nbk_key_map[SDLK_BACKSLASH] = 0;

    nbk_key_map[SDLK_SEMICOLON] = SDLK_COLON; // 分号
    nbk_key_map[SDLK_QUOTE] = SDLK_QUOTEDBL;

    nbk_key_map[SDLK_COMMA] = SDLK_LESS; // 逗号
    nbk_key_map[SDLK_PERIOD] = SDLK_GREATER;
    nbk_key_map[SDLK_SLASH] = SDLK_QUESTION;
}

void runtime_init(void)
{
	if (l_sem == N_NULL)
		l_sem = SDL_CreateSemaphore(1);

    key_map_init();
}

void runtime_end(void)
{
	if (l_sem) {
		SDL_DestroySemaphore(l_sem);
		l_sem = N_NULL;
	}

	fprintf(stderr, "\n>>> Mem alloc: %d free: %d leak: %d", l_mem_alloc, l_mem_free, l_mem_alloc - l_mem_free);
    fprintf(stderr, "\n>>> Max timers used: %d\n", l_max_timers);

#ifdef MEM_TRACK
	mem_track_leak();
#endif
}

void sync_wait(const char* caller)
{
    //printf("sync %ld %s ====\n", ++l_sync_count, caller);
    SDL_SemWait(l_sem);
    //printf("sync %ld %s --->\n", ++l_sync_count, caller);
}

void sync_post(const char* caller)
{
    //printf("sync %ld %s <---\n", l_sync_count, caller);
    SDL_SemPost(l_sem);
}

void* NBK_malloc(size_t size)
{
	void* ptr = malloc(size);
	l_mem_alloc++;
#ifdef MEM_TRACK
	mem_track_add(ptr, size);
#endif
	return ptr;
}

void* NBK_malloc0(size_t size)
{
	void* ptr = calloc(size, 1);
	l_mem_alloc++;
#ifdef MEM_TRACK
	mem_track_add(ptr, size);
#endif
	return ptr;
}

void* NBK_realloc(void* ptr, size_t size)
{
	void* p;
#ifdef MEM_TRACK
	mem_track_remove(ptr);
#endif
	p = realloc(ptr, size);
#ifdef MEM_TRACK
	mem_track_add(p, size);
#endif
	return p;
}

void NBK_free(void* ptr)
{
#ifdef MEM_TRACK
	mem_track_remove(ptr);
#endif
	l_mem_free++;
	free(ptr);
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

int NBK_atoi(const char* s)
{
	return atoi(s);
}

uint32 NBK_htoi(const char* s)
{
	uint32 v = 0;
	int len = nbk_strlen(s);
	int i, j, n, base;
	for (i = 0; i < len; i++) {
		for (j = 0, n = len-i-1, base = 1; j < n; j++)
			base *= 16;
		if (s[i] >= 'a' && s[i] <= 'f')
			n = 10 + s[i] - 'a';
		else if (s[i] >= 'A' && s[i] <= 'F')
			n = 10 + s[i] - 'A';
		else
			n = s[i] - '0';
		v += n * base;
	}
	return v;
}

NFloat NBK_atof(const char* s)
{
	NFloat nf;
	double f = atof(s);
	nf.i = (int)f;
	nf.f = (int)((f - nf.i) * NFLOAT_EMU);
	return nf;
}

void NBK_md5(const char* s, int len, uint8* sig)
{
	md5_t md5;
	md5_init(&md5);
	md5_process(&md5, s, len);
	md5_finish(&md5, sig);
}

int NBK_currentMilliSeconds(void)
{
	return (int)SDL_GetTicks();
}

int NBK_freemem(void)
{
	return 8000000;
}

// -----------------------------------------------------------------------------
// 输入法
// -----------------------------------------------------------------------------

void NBK_fep_enable(void* pfd)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbk->inEditing = N_TRUE;
}

void NBK_fep_disable(void* pfd)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbk->inEditing = N_FALSE;
}

void NBK_fep_updateCursor(void* pfd)
{
}

// -----------------------------------------------------------------------------
// 编码转换
// -----------------------------------------------------------------------------

int NBK_conv_gbk2unicode(const char* mbs, int mbLen, wchr* wcs, int wcsLen)
{
	int count = 0;
#ifdef PLATFORM_WIN32
	_locale_t loc = _create_locale(LC_ALL, "Chinese");
	if (loc) {
		char* m = (char*)NBK_malloc(mbLen + 1);
		nbk_strncpy(m, mbs, mbLen);
		count = _mbstowcs_l((wchar_t*)wcs, m, mbLen, loc);
		NBK_free(m);
		__free_locale(loc);
	}
#endif
	return count;
}

// -----------------------------------------------------------------------------
// 定计器组
// -----------------------------------------------------------------------------

static int timerMgr_getActiveTimer(NBK_timerMgr* mgr, int* num)
{
	int i, id = -1;
	Uint32 time = 0xfffffffe;

    *num = 0;
	for (i=0; i < MAX_TIMERS; i++) {
		if (mgr->lst[i].active && mgr->lst[i].ready) {
            (*num)++;
			if (mgr->lst[i].start < time) { // 选取最早启动的定时器
				id = i;
				time = mgr->lst[i].start;
			}
		}
	}

	return id;
}

//static nbool l_process_timers = N_FALSE;

static Uint32 timerMgr_schedule(Uint32 interval, void* param)
{
	NBK_timerMgr* mgr = (NBK_timerMgr*)param;
	int i, active;

	sync_wait("Timer");
    //l_process_timers = N_TRUE;

	active = 0;
	for (i=0; i < MAX_TIMERS; i++) {
		if (mgr->lst[i].active) {
			mgr->lst[i].elapse += interval;
			if (mgr->lst[i].elapse >= mgr->lst[i].delay) {
				mgr->lst[i].delay = mgr->lst[i].interval;
                mgr->lst[i].ready = N_TRUE;
			    active++;
			}
		}
	}

    //if (active) {
    //    fprintf(stderr, "  %d timers available\n", active);
    //}

	while (active > 0) {
		// 选取最早启动的定时器
		i = timerMgr_getActiveTimer(mgr, &active);
		if (i == -1)
			break;
		mgr->lst[i].t->func(mgr->lst[i].t->user);
		mgr->lst[i].elapse = 0;
		mgr->lst[i].start = SDL_GetTicks();
        mgr->lst[i].ready = N_FALSE;
	};

	sync_post("Timer");
    //l_process_timers = N_FALSE;

	return interval;
}

//nbool runtime_processTimers(void)
//{
//    return l_process_timers;
//}

NBK_timerMgr* timerMgr_create(void)
{
	NBK_timerMgr* mgr = (NBK_timerMgr*)NBK_malloc0(sizeof(NBK_timerMgr));
	return mgr;
}

void timerMgr_delete(NBK_timerMgr** timerMgr)
{
	NBK_timerMgr* mgr = *timerMgr;
	//if (mgr->id != NULL)
	//	SDL_RemoveTimer(mgr->id);
	NBK_free(mgr);
	*timerMgr = N_NULL;
}

void timerMgr_createTimer(NBK_timerMgr* timerMgr, NTimer* timer)
{
	int i;

	for (i=1; i < MAX_TIMERS; i++) {
		if (timerMgr->lst[i].t == N_NULL) {
			timerMgr->lst[i].t = timer;
			timerMgr->lst[i].elapse = 0;
			timerMgr->lst[i].active = N_FALSE;
            timerMgr->lst[i].ready = N_FALSE;
			timer->id = i;
            l_max_timers = N_MAX(l_max_timers, i);
			//fprintf(stderr, "add timer %d\n", i);
			return;
		}
	}

	N_KILL_ME();
}

void timerMgr_removeTimer(NBK_timerMgr* timerMgr, NTimer* timer)
{
	if (timer->id > 0 && timer->id < MAX_TIMERS) {
		timerMgr->lst[timer->id].t = N_NULL;
		timerMgr->lst[timer->id].active = N_FALSE;
        timerMgr->lst[timer->id].ready = N_FALSE;
		//fprintf(stderr, "remove timer %d\n", timer->id);
	}
}

void timerMgr_startTimer(NBK_timerMgr* timerMgr, NTimer* timer, int delay, int interval)
{
	if (timer->id > 0 && timer->id < MAX_TIMERS) {
		timerMgr->lst[timer->id].delay = delay;
		timerMgr->lst[timer->id].interval = interval;
		timerMgr->lst[timer->id].elapse = 0;
		timerMgr->lst[timer->id].active = N_TRUE;
        timerMgr->lst[timer->id].ready = N_FALSE;
		timerMgr->lst[timer->id].start = SDL_GetTicks();

        timerMgr->active = N_TRUE;

		//if (timerMgr->id == NULL) {
		//	//fprintf(stderr, "create SDL timer\n");
		//	timerMgr->id = SDL_AddTimer(SDL_TIMESLICE * 5, timerMgr_schedule, timerMgr);
		//}
	}
}

void timerMgr_stopTimer(NBK_timerMgr* timerMgr, NTimer* timer)
{
	int i;

	if (timer->id > 0 && timer->id < MAX_TIMERS) {
		timerMgr->lst[timer->id].active = N_FALSE;
        timerMgr->lst[timer->id].ready = N_FALSE;

		//if (timerMgr->id == NULL)
		//	return;

		for (i=0; i < MAX_TIMERS; i++)
			if (timerMgr->lst[i].active)
				break;

		if (i == MAX_TIMERS) { // 当无活跃定时器时，删除真实定时器
            timerMgr->active = N_FALSE;

			//fprintf(stderr, "remove SDL timer\n");
			//SDL_RemoveTimer(timerMgr->id);
			//timerMgr->id = NULL;
		}
	}
}

void timerMgr_run(NBK_timerMgr* timerMgr)
{
    if (timerMgr->active)
        timerMgr_schedule(20, timerMgr);
}

void NBK_timerCreate(NTimer* timer)
{
	timerMgr_createTimer(g_nbk_core->timerMgr, timer);
}

void NBK_timerDelete(NTimer* timer)
{
	timerMgr_removeTimer(g_nbk_core->timerMgr, timer);
}

void NBK_timerStart(NTimer* timer, int delay, int interval)
{
	timerMgr_startTimer(g_nbk_core->timerMgr, timer, delay, interval);
}

void NBK_timerStop(NTimer* timer)
{
	timerMgr_stopTimer(g_nbk_core->timerMgr, timer);
}

// -----------------------------------------------------------------------------
// 平台文件系统函数
// -----------------------------------------------------------------------------

nbool nbk_pathExist(const char* path)
{
	int ret = 0;
#ifdef PLATFORM_WIN32
	ret = _access(path, 0);
#endif
#ifdef PLATFORM_LINUX
	ret = access(path, 0);
#endif
	return (ret == 0) ? N_TRUE : N_FALSE;
}

nbool nbk_makeDir(const char* path)
{
	int ret;
	char* dir;
	char* p;
	char deli;

	dir = (char*)NBK_malloc(nbk_strlen(path) + 1);
	nbk_strcpy(dir, path);

	if (*(dir+1) == ':')
		p = dir + 3; // 跳过盘符
	else if (*dir == '\\' || *dir == '/')
		p = dir + 1; // 跳过根
	else
		p = dir;

	while (*p) {
		if (*p == '\\' || *p == '/') {
			deli = *p;
			*p = 0;
#ifdef PLATFORM_WIN32
			ret = _mkdir(dir);
#endif
#ifdef PLATFORM_LINUX
			ret = mkdir(dir, S_IRWXU);
#endif
			*p = deli;
		}
		p++;
	}

	NBK_free(dir);
	return N_TRUE;
}

void nbk_removeDir(const char* path)
{
	int err;
#ifdef PLATFORM_WIN32
	intptr_t handle;
	struct _finddata_t data;

	if (_chdir(path) != 0)
		return;

	handle = _findfirst("*.*", &data);
	if (handle != -1) {
		do {
			if (!(data.attrib & _A_SUBDIR))
				err = _unlink(data.name);
		} while (_findnext(handle, &data) == 0);
		err = _findclose(handle);
		err = _chdir("..");
		err = _rmdir(path);
	}
#endif
#ifdef PLATFORM_LINUX
	DIR* dirp;
	struct stat statbuf;
	struct dirent* dp;

	dirp = opendir(path);
	if (dirp == 0)
		return;
	err = chdir(path);
	while ((dp = readdir(dirp)) != NULL) {
		if (stat(dp->d_name, &statbuf) == -1)
			continue;
		if (S_ISDIR(statbuf.st_mode))
			continue;
		unlink(dp->d_name);
	}
	err = chdir("..");
	closedir(dirp);
	err = rmdir(path);
#endif
}

void nbk_removeMultiDir(const char* path, const char* match)
{
	int err;
#ifdef PLATFORM_WIN32
	intptr_t handle;
	struct _finddata_t data;

	if (_chdir(path) != 0)
		return;

	handle = _findfirst(match, &data);
	if (handle != -1) {
		do {
			if (data.attrib & _A_SUBDIR)
				nbk_removeDir(data.name);
		} while (_findnext(handle, &data) == 0);
		err = _findclose(handle);
	}
#endif
#ifdef PLATFORM_LINUX
	glob_t globbuf;
	struct stat statbuf;
	int i;

	err = chdir(path);
	if (err)
		return;

	err = glob(match, 0, N_NULL, &globbuf);
	if (err == 0) {
		for (i=0; i < globbuf.gl_pathc; i++) {
			if (stat(globbuf.gl_pathv[i], &statbuf) == -1)
				continue;
			if (S_ISDIR(statbuf.st_mode))
				nbk_removeDir(globbuf.gl_pathv[i]);
		}
		globfree(&globbuf);
	}
#endif
}

void nbk_removeFile(const char* fname)
{
	int err;
#ifdef PLATFORM_WIN32
	err = _unlink(fname);
#endif
#ifdef PLATFORM_LINUX
	err = unlink(fname);
#endif
}
