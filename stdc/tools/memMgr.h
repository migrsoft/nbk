#ifndef __MEMMGR_H__
#define __MEMMGR_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

void memMgr_init(void);
void memMgr_quit(void);

void* node_alloc(void);
void  node_free(void* ptr);

void* render_alloc(void);
void  render_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif // __MEMMGR_H__
