#ifndef __NBK_CACHEMGR_H__
#define __NBK_CACHEMGR_H__

// 文件缓存管理器
//
// 功能：缓存最常用的资源文件。
// 策略：仅在同一资源被查询一定次数以上，再进行缓存。

#include "../inc/config.h"
#include "../tools/hashMap.h"
#include "../tools/strBuf.h"

#ifdef __cplusplus
extern "C" {
#endif

enum NECacheType {
	NECACHET_UNKNOWN,
	NECACHET_WMLC,
	NECACHET_HTM,
	NECACHET_HTM_GZ,
	NECACHET_CSS,
	NECACHET_CSS_GZ,
	NECACHET_JS,
	NECACHET_JS_GZ,
	NECACHET_GIF,
	NECACHET_JPG,
	NECACHET_PNG,
	NECACHET_LAST
};

typedef struct _NCacheEntry {

	char*	url;
	uint8	type;
	uint8	encoding;
	uint16	lru;
	uint32	time;
	int		size;

	// 内存数据
	NStrBuf*	buf;
	FILE*		fd;

} NCacheEntry;

typedef struct _NCacheMgr {

	char*			path;

	NCacheEntry*	tab;
	NHashMap*		map;

	int		capa;
	int		use;

} NCacheMgr;

NCacheMgr* cacheMgr_create(int capacity);
void cacheMgr_delete(NCacheMgr** mgr);

void cacheMgr_setPath(NCacheMgr* mgr, const char* path);
void cacheMgr_load(NCacheMgr* mgr);
void cacheMgr_save(NCacheMgr* mgr);

void cacheMgr_dump(NCacheMgr* mgr);

// 存储文件
int cacheMgr_store(NCacheMgr* mgr, const char* url, uint8 type, uint8 encoding);
void cacheMgr_putData(NCacheMgr* mgr, int cacheId, char* data, int length, nbool over);

// 检索文件
int cacheMgr_find(NCacheMgr* mgr, const char* url, uint8* type, uint8* encoding, int* length);
int cacheMgr_getData(NCacheMgr* mgr, int cacheId, char* buf, int size, int from);

#ifdef __cplusplus
}
#endif

#endif //__NBK_CACHEMGR_H__
