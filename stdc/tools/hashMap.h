/*
 * Hash Map
 *
 * Created on: 2012-11-29
 *     Author: wuyulun
 */

#ifndef __N_HASHMAP_H__
#define __N_HASHMAP_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NHashEntry {
	char*	key;
	void*	value;
	void*	next;
} NHashEntry;

typedef struct _NHashMap {
	NHashEntry*		lst;
	int				size;
} NHashMap;

// 创建HASHMAP，capacity 为一级表的容量
NHashMap* hashMap_create(int capacity);
void hashMap_delete(NHashMap** map);

void* hashMap_put(NHashMap* map, const char* key, void* value);
nbool hashMap_get(NHashMap* map, const char* key, void** value);
void* hashMap_remove(NHashMap* map, const char* key);

void hashMap_clear(NHashMap* map);

void hashMap_dump(NHashMap* map);

#ifdef __cplusplus
}
#endif

#endif // __N_HASHMAP_H__
