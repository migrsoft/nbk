#ifndef __NBK_CONFIG_H__
#define __NBK_CONFIG_H__

#include <stdio.h>
#include "../tools/smartPtr.h"

// features
//#define NBK_USE_BIGENDIAN
#define NBK_USE_MEMMGR

#define NBK_EXT_SHORTCUT_MENU       0
#define NBK_EXT_MAINBODY_MODE       0

#define NBK_DISABLE_ZOOM            0
#define NBK_ENABLE_CBS				0

#define CRYPTO_DATA                 0 // enabel en/decrypt

//#define NBK_MEM_TEST
#define	DOM_TEST_INFO				1 // 结点加入编号

#ifdef __cplusplus
extern "C" {
#endif

/*
 * basic data types
 */

typedef char				int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;
typedef int					int32;
typedef unsigned int		uint32;

typedef unsigned int		size_t;
typedef unsigned short      wchr;

typedef int8                nbool;
typedef int32               coord;
typedef uint16              nid;

#define N_TRUE  1
#define N_FALSE 0
#define N_NULL  0L
#define N_MAX(a, b) ((a > b) ? a : b)
#define N_MIN(a, b) ((a < b) ? a : b)
#define N_ABS(n) ((n >= 0) ? n : -n)
#define N_INVALID_ID    0xffff
#define N_MAX_UINT      0xffffffff
#define N_INVALID_COORD 12321
#define N_MAX_COORD     100000

#define N_KILL_ME() {char* crash = N_NULL; *crash = 88;}
#define N_ASSERT(e) {if (!(e)) {char* crash=N_NULL; *crash=88;}}

#ifdef NBK_USE_BIGENDIAN
#define N_SWAP_UINT16(n)    ((n >> 8 & 0xff) | (n << 8 & 0xff00))
#define N_SWAP_UINT32(n)    ((n >> 24 & 0xff) | (n >> 8 & 0xff00) | \
                             (n << 8 & 0xff0000) | (n << 24 & 0xff000000)) 
#else
#define N_SWAP_UINT16(n)    (n)
#define N_SWAP_UINT32(n)    (n)
#endif

extern void* g_dp;

int32 float_imul(int32 src, float factor);
int32 float_idiv(int32 src, float factor);

/*
 * memory interface
 */
 
void* NBK_malloc(size_t size);
void* NBK_malloc0(size_t size);
void* NBK_realloc(void* ptr, size_t size);
void NBK_free(void* p);

void NBK_memcpy(void* dst, void* src, size_t size);
void NBK_memset(void* dst, int8 value, size_t size);
void NBK_memmove(void* dst, void* src, size_t size);

uint32 NBK_currentMilliSeconds(void);
uint32 NBK_currentSeconds(void);
uint32 NBK_currentFreeMemory(void); // total free memory in bytes

void NBK_fep_enable(void* pfd);
void NBK_fep_disable(void* pfd);
void NBK_fep_updateCursor(void* pfd);

//int NBK_conv_gbk2unicode(const char* mbs, int mbsLen, wchr* wcs, int wcsLen);

void NBK_unlink(const char* path);

#ifdef __cplusplus
}
#endif

#endif // __NBK_CONFIG_H__
