/*
 * memory.c
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#include "../../../stdc/inc/config.h"
#include <e32std.h>
#include <hal.h>
#include <f32file.h>
#include <charconv.h>

#ifdef USE_MM32
#include <DXMemoryManager.h>
using namespace DXBrowser;
#endif

void* NBK_malloc(size_t size)
{
#ifdef USE_MM32
    return DXMemoryManager::Alloc(size);
#else
    return User::Alloc(size);
#endif
}

void* NBK_malloc0(size_t size)
{
#ifdef USE_MM32
    void* p = DXMemoryManager::Alloc(size);
    if (p)
        NBK_memset(p, 0, size);
    return p;
#else
    return User::AllocZ(size);
#endif
}

void* NBK_realloc(void* ptr, size_t size)
{
#ifdef USE_MM32
    return DXMemoryManager::ReAlloc(ptr, size);
#else
    return User::ReAlloc(ptr, size);
#endif
}

void NBK_free(void* p)
{
#ifdef USE_MM32
    DXMemoryManager::Free(p);
#else
    return User::Free(p);
#endif
}

void NBK_memcpy(void* dst, void* src, size_t size)
{
    Mem::Copy(dst, src, size);
}

void NBK_memset(void* dst, int8 value, size_t size)
{
    Mem::Fill(dst, size, value);
}

void NBK_memmove(void* dst, void* src, size_t size)
{
    Mem::Move(dst, src, size);
}

// 当前毫秒数(1/1000s)
uint32 NBK_currentMilliSeconds(void)
{
    return User::NTickCount();
}

uint32 NBK_currentSeconds(void)
{
    return User::NTickCount() / 1000;
}

// 检测当前可用内存数，单位为字节
uint32 NBK_currentFreeMemory(void)
{
    int free = 5000000;
    HAL::Get(HALData::EMemoryRAMFree, free);
    return (uint32)free;
}

// 转换GBK编码为unicode
// mbs: gbk编码字符串
// mbLen: gbk编码字符串长度
// wcs: 用于存储转换后的unicode字符串的内存
// wcsLen: 可供存储的unicode字符数量
// 返回为转换后的unicode字符数量
int NBK_conv_gbk2unicode(const char* mbs, int mbLen, wchr* wcs, int wcsLen)
{
    int wcl = 0;
    RFs rfs;
    rfs.Connect();
    CCnvCharacterSetConverter* conv = CCnvCharacterSetConverter::NewLC();
    if (conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierGbk, rfs)
        == CCnvCharacterSetConverter::EAvailable) {
        TInt state = CCnvCharacterSetConverter::KStateDefault;
        TPtrC8 gbk((TUint8*)mbs, mbLen);
        TPtr ucs(wcs, 0, wcsLen);
        conv->ConvertToUnicode(ucs, gbk, state);
        wcl = ucs.Length();
    }
    CleanupStack::PopAndDestroy();
    rfs.Close();
    return wcl;
}
