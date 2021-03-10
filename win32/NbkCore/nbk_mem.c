#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <locale.h>
#include <tchar.h>
#include <io.h>
#include "nbk_mem.h"
#include "../../stdc/inc/config.h"
#include "../../stdc/tools/str.h"

static int l_mem_alloc = 0;
static int l_mem_free = 0;

void memory_info(void)
{
	TCHAR msg[64];
	wsprintf(msg, _T("*** MEM alloc: %d free: %d ***\n"), l_mem_alloc, l_mem_free);
	OutputDebugString(msg);
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
	return GetTickCount();
}

uint32 NBK_currentSeconds(void)
{
	return (uint32)(GetTickCount64() / 1000);
}

uint32 NBK_currentFreeMemory(void)
{
	return 10000000;
}

void NBK_fep_enable(void* pfd)
{
}

void NBK_fep_disable(void* pfd)
{
}

void NBK_fep_updateCursor(void* pfd)
{
}

//int NBK_conv_gbk2unicode(const char* mbs, int mbLen, wchr* wcs, int wcsLen)
//{
//	int count = 0;
//	_locale_t loc = _create_locale(LC_ALL, "Chinese");
//	if (loc) {
//		char* m = (char*)NBK_malloc(mbLen + 1);
//		nbk_strncpy(m, mbs, mbLen);
//		//count = _mbstowcs_l(NULL, m, mbLen, loc);
//		count = _mbstowcs_l((wchar_t*)wcs, m, mbLen, loc);
//		NBK_free(m);
//		_free_locale(loc);
//	}
//	return count;
//}

void NBK_unlink(const char* path)
{
	_unlink(path);
}

////////////////////////////////////////////////////////////////////////////////
//
// ×Ö·û×ª»»
//

int NBK_atoi(const char* s)
{
	return atoi(s);
}

float NBK_atof(const char* s)
{
	return (float)atof(s);
}
