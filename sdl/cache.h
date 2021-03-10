#ifndef __NBK_CACHE_H__
#define __NBK_CACHE_H__

#include "../stdc/inc/config.h"
#include "../stdc/tools/slist.h"
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NECacheType {
	NECACHET_UNKNOWN,
	NECACHET_CSS,
	NECACHET_CSS_GZ,
	NECACHET_PNG,
	NECACHET_GIF,
	NECACHET_JPG,
	NECACHET_HTM
};

typedef struct _NCache {
	int		id;
	int		leave;
	NSList*	lst;
} NCache;

typedef struct _NCacheItem {
	nid		type;
	nid		rese1;
	uint8	sig[16];	// uri MD5
	time_t	time;
	//int		lru;
	//int		block;		// 块号
	uint32	dataLen;	// 数据长度
	uint32	userLen;	// 额外数据长度
} NCacheItem;

typedef struct _NMapItem {
	uint8	state;
	uint8	rese1;
	uint8	rese2;
	uint8	rese3;
	int		nextBlock;
} NMapItem;

typedef struct _NCacheMgr {
	NCacheItem*	idx;	// 缓存索引表
	//NMapItem*	map;	// 数据块信息表
	//FILE*		dat;
	NSList*		lst;	// 未写的缓存数据
} NCacheMgr;

NCacheMgr* cacheMgr_create(void);
void cacheMgr_delete(NCacheMgr** mgr);

int cacheMgr_save(NCacheMgr* mgr, const char* uri, nid type);
void cacheMgr_saveData(NCacheMgr* mgr, int id, uint8* data, int len);
void cacheMgr_saveOver(NCacheMgr* mgr, int id, nbool save);

int cacheMgr_load(NCacheMgr* mgr, const char* uri);
nid cacheMgr_loadDataInfo(NCacheMgr* mgr, int id, int* dataLen, int* userLen);
int cacheMgr_loadData(NCacheMgr* mgr, int id, int offset, void* buf, int size);

#ifdef __cplusplus
}
#endif

#endif
