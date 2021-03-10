// create by wuyulun, 2012.1.20

#include "nbk_conf.h"
#include "../stdc/tools/str.h"
#include "cache.h"
#include "resmgr.h"
#include "runtime.h"
#include "md5.h"
#include "ini.h"
#ifdef PLATFORM_LINUX
#include <unistd.h>
#endif

//#define DEBUG_CACHE

#define MAX_ITEMS	2000 // 最大缓存数
#define MAX_BLOCKS	512
#define BLOCK_SIZE	16384

#define MAX_BLOCK_NO	MAX_BLOCKS
#define MAX_LRU			0x80000000L
#define MAX_TIME_KEEP	1800 // 30分钟

#define CACHE_VER	1
#define CACHE_IDX	"cache.idx"
#define CACHE_MAP	"cache.map"
#define CACHE_DAT	"cache.dat"

#define FSCACHE_PATH_FMT		"%sfsc/"
#define FSCACHE_SUBPATH_FMT		"%sfsc/%c/"
#define FSCACHE_INDEX_FMT		"%sfsc/%s"
#define FSCACHE_DATA_FMT		"%sfsc/%c/%s"

#define HASH_PHI	0x9e3779b9U

enum NECacheMap {
	NECMAP_FREE,
	NECMAP_USED,
	NECMAP_RESE
};

static NCache* cache_create(int id)
{
	NCache* c = (NCache*)NBK_malloc0(sizeof(NCache));
	c->id = id;
	c->lst = sll_create();
	return c;
}

static void cache_delete(NCache** cache)
{
	NCache* c = *cache;
	uint8* p = (uint8*)sll_first(c->lst);
	while (p) {
		NBK_free(p);
		p = (uint8*)sll_next(c->lst);
	}
	sll_delete(&c->lst);
	NBK_free(c);
	*cache = N_NULL;
}

static void cache_addData(NCache* cache, uint8* data, int len)
{
	uint8* p;
	int n, size, offs;

	offs = 0;
	size = len;
	while (size) {
		if (cache->leave == 0) {
			p = (uint8*)NBK_malloc0(BLOCK_SIZE);
			cache->leave = BLOCK_SIZE;
			sll_append(cache->lst, p);
		}
		else {
			p = (uint8*)sll_last(cache->lst);
			p = p + BLOCK_SIZE - cache->leave;
		}
		n = (size >= cache->leave) ? cache->leave : size;
		NBK_memcpy(p, data + offs, n);
		offs += n;
		cache->leave -= n;
		size -= n;
	}
}

static int cache_getBlockNum(NCache* cache)
{
	int num = 0;
	uint8* p = (uint8*)sll_first(cache->lst);
	while (p) {
		num++;
		p = (uint8*)sll_next(cache->lst);
	}
	return num;
}

static void cache_genFilePath(char* path, uint8* sig)
{
	char md5str[34];
	md5_sig_to_string(sig, md5str, 34);
	sprintf(path, FSCACHE_DATA_FMT, ini_getString(NEINI_DATA_PATH), md5str[0], md5str);
}

static int index_gen(const char* uri)
{
	uint32 len = nbk_strlen(uri);
	uint32 h = HASH_PHI;
	uint32 i;

    h += len;
    h += (h << 10);
    h ^= (h << 6);
	if (len) {
        for (i = 0; i < len; i++) {
            h += uri[i];
            h += (h << 10);
            h ^= (h << 6);
        }
    }
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);

    h = h % MAX_ITEMS;
	return (int)h;
}

static nbool index_compare_sig(uint8* sig1, uint8* sig2)
{
	int i;
	for (i=0; i < 16; i++) {
		if (sig1[i] != sig2[i])
			return N_FALSE;
	}
	return N_TRUE;
}

//static int index_get_min_lru(NCacheItem* table)
//{
//	int lru = MAX_LRU;
//	int i;
//	for (i=0; i < MAX_ITEMS; i++) {
//		if ((table[i].block >= 0 && table[i].block < MAX_BLOCK_NO) && table[i].lru < lru)
//			lru = table[i].lru;
//	}
//	return lru;
//}

NCacheMgr* cacheMgr_create(void)
{
	NCacheMgr* mgr = (NCacheMgr*)NBK_malloc0(sizeof(NCacheMgr));
	int i;
	char path[MAX_CACHE_PATH_LEN];
	FILE* fd;

	sprintf(path, FSCACHE_PATH_FMT, ini_getString(NEINI_DATA_PATH));
	if (!nbk_pathExist(path))
		nbk_makeDir(path);

	mgr->idx = (NCacheItem*)NBK_malloc0(sizeof(NCacheItem) * MAX_ITEMS);
	sprintf(path, FSCACHE_INDEX_FMT, ini_getString(NEINI_DATA_PATH), CACHE_IDX);
	fd = fopen(path, "rb");
	if (fd) {
		int ver;
		fread(&ver, sizeof(int), 1, fd);
		fread(mgr->idx, sizeof(NCacheItem), MAX_ITEMS, fd);
		fclose(fd);
	}
	//for (i=0; i < MAX_ITEMS; i++)
	//	mgr->idx[i].block = -1;

	//mgr->map = (NMapItem*)NBK_malloc0(sizeof(NMapItem) * MAX_BLOCKS);

	//sprintf(path, "%s%s", ini_getString(NEINI_DATA_PATH), CACHE_DAT);
	//mgr->dat = fopen(path, "a+b");

	mgr->lst = sll_create();

	return mgr;
}

void cacheMgr_delete(NCacheMgr** mgr)
{
	NCacheMgr* m = *mgr;
	NCache* c;
	char path[MAX_CACHE_PATH_LEN];
	FILE* fd;

	sprintf(path, FSCACHE_INDEX_FMT, ini_getString(NEINI_DATA_PATH), CACHE_IDX);
	fd = fopen(path, "wb");
	if (fd) {
		int ver = CACHE_VER;
		fwrite(&ver, sizeof(int), 1, fd);
		fwrite(m->idx, sizeof(NCacheItem), MAX_ITEMS, fd);
		fclose(fd);
	}
	NBK_free(m->idx);

	//sprintf(path, "%s%s", ini_getString(NEINI_DATA_PATH), CACHE_MAP);
	//fd = fopen(path, "wb");
	//if (fd) {
	//	fwrite(m->map, sizeof(NMapItem), MAX_BLOCKS, fd);
	//	fclose(fd);
	//}
	//NBK_free(m->map);

	//if (m->dat)
	//	fclose(m->dat);

	c = (NCache*)sll_first(m->lst);
	while (c) {
		cache_delete(&c);
		c = (NCache*)sll_next(m->lst);
	}
	sll_delete(&m->lst);

	NBK_free(m);
	*mgr = N_NULL;
}

int cacheMgr_save(NCacheMgr* mgr, const char* uri, nid type)
{
	time_t now;
	int pos = index_gen(uri);
	NCacheItem* item = &mgr->idx[pos];
	NCache* cache;

	time(&now);

	if (   item->time > 0
		&& now - item->time < MAX_TIME_KEEP )
    {
#ifdef DEBUG_CACHE
        fprintf(stderr, "cache (%d) failed %s\n", pos, uri);
#endif
		return -1;
    }

	if (item->dataLen) {
		int err;
		char path[MAX_CACHE_PATH_LEN];
		cache_genFilePath(path, item->sig);
#ifdef PLATFORM_WIN32
		err = _unlink(path);
#endif
#ifdef PLATFORM_LINUX
		err = unlink(path);
#endif
	}

	item->type = type;
	NBK_md5(uri, nbk_strlen(uri), item->sig);
	item->time = now;
	item->dataLen = item->userLen = 0;

	cache = cache_create(pos);
	sll_append(mgr->lst, cache);

	return pos;
}
//{
//	int pos = index_gen(uri);
//	NCacheItem* table = mgr->idx;
//	NCacheItem* item;
//	NCache* cache;
//
//	//fprintf(stderr, "HASH %s -> %d\n", uri, pos);
//
//	if (table[pos].block == MAX_BLOCK_NO)
//		return -1;
//
//	if (table[pos].block != -1) {
//		NMapItem* map = mgr->map;
//		int bn = table[pos].block;
//		while (bn != -1) {
//			map[bn].state = NECMAP_FREE;
//			bn = map[bn].nextBlock;
//		}
//	}
//
//	item = &table[pos];
//	NBK_md5(uri, nbk_strlen(uri), item->sig);
//	item->lru = 0;
//	item->block = MAX_BLOCK_NO;
//	item->dataLen = item->userLen = 0;
//
//	cache = cache_create(pos);
//	sll_append(mgr->lst, cache);
//	return pos;
//}

static NCache* cacheMgr_getCache(NCacheMgr* mgr, int id)
{
	NCache* c = (NCache*)sll_first(mgr->lst);
	while (c) {
		if (c->id == id)
			break;
		c = (NCache*)sll_next(mgr->lst);
	}
	return c;
}

static void cacheMgr_removeCache(NCacheMgr* mgr, int id)
{
	NCache* c = (NCache*)sll_first(mgr->lst);
	while (c) {
		if (c->id == id) {
			cache_delete(&c);
			sll_removeCurr(mgr->lst);
			break;
		}
		c = (NCache*)sll_next(mgr->lst);
	}
}

//static int* cacheMgr_findSaveBlocks(NCacheMgr* mgr, int num)
//{
//	int* blks = (int*)NBK_malloc(sizeof(int) * (num + 1));
//	int i, j, lru, bn;
//	NCacheItem* table = mgr->idx;
//	NMapItem* map = mgr->map;
//
//	// 查找空闲块
//	for (i = j = 0; i < MAX_BLOCKS && j < num; i++) {
//		if (map[i].state == NECMAP_FREE) {
//			blks[j++] = i;
//			map[i].state = NECMAP_RESE;
//		}
//	}
//	if (j == num)
//		return blks;
//
//	// 释放现有块
//	while (1) {
//		lru = index_get_min_lru(table);
//		if (lru == MAX_LRU)
//			break;
//
//		for (i=0; i < MAX_ITEMS && j < num; i++) {
//			if ((table[i].block >= 0 && table[i].block < MAX_BLOCK_NO) && table[i].lru == lru) {
//				bn = table[i].block;
//				table[i].block = -1;
//
//				while (bn != -1) {
//					if (j < num)
//						blks[j++] = bn;
//					map[bn].state = NECMAP_RESE;
//					bn = map[bn].nextBlock;
//				}
//			}
//		}
//	}
//
//	if (lru == MAX_LRU) { // 无法获取足够的空间
//		for (i=0; i < j; i++) {
//			map[blks[i]].state = NECMAP_FREE;
//		}
//		NBK_free(blks);
//		blks = N_NULL;
//	}
//
//	return blks;
//}

void cacheMgr_saveData(NCacheMgr* mgr, int id, uint8* data, int len)
{
	NCache* c = cacheMgr_getCache(mgr, id);
	if (c) {
		cache_addData(c, data, len);
		mgr->idx[id].dataLen += len;
	}
}

void cacheMgr_saveOver(NCacheMgr* mgr, int id, nbool save)
{
	NCache* c = cacheMgr_getCache(mgr, id);
	if (c) {
		char path[MAX_CACHE_PATH_LEN];
		char str[4];
		FILE* fd;
		uint8* p;
		int num = cache_getBlockNum(c);
		int i;
		nbool use = N_FALSE;

		if (num && save) {
			sprintf(str, "%x", mgr->idx[id].sig[0]);

			sprintf(path, FSCACHE_SUBPATH_FMT, ini_getString(NEINI_DATA_PATH), str[0]);
			if (!nbk_pathExist(path))
				nbk_makeDir(path);

			cache_genFilePath(path, mgr->idx[id].sig);
			fd = fopen(path, "wb");
			if (fd) {
				p = (uint8*)sll_first(c->lst);
				i = 1;
				while (p) {
					fwrite(p, 1, ((i == num) ? BLOCK_SIZE - c->leave : BLOCK_SIZE), fd);
					p = (uint8*)sll_next(c->lst);
					i++;
				}
				fclose(fd);
				use = N_TRUE;
			}
		}

		if (!use)
			NBK_memset(&mgr->idx[id], 0, sizeof(NCacheItem));

		cacheMgr_removeCache(mgr, id);
	}
}
//{
//	NCache* c = cacheMgr_getCache(mgr, id);
//	int num;
//	int* blks;
//
//	if (c == N_NULL)
//		return;
//
//	num = cache_getBlockNum(c);
//	if (num && mgr->dat && save) {
//		blks = cacheMgr_findSaveBlocks(mgr, num);
//		if (blks) {
//			int i = 0;
//			int offs;
//			char ch = 0;
//			uint8* p = (uint8*)sll_first(c->lst);
//			NMapItem* map = mgr->map;
//
//			// 将数据块写入文件
//			while (p && i < num) {
//				offs = blks[i] * BLOCK_SIZE;
//				fseek(mgr->dat, offs, SEEK_SET);
//				fwrite(p, 1, BLOCK_SIZE, mgr->dat);
//				map[blks[i]].state = NECMAP_USED;
//				map[blks[i]].nextBlock = (i < num - 1) ? blks[i+1] : -1;
//				p = (uint8*)sll_next(c->lst);
//				i++;
//			}
//			mgr->idx[id].block = blks[0];
//			NBK_free(blks);
//		}
//		else
//			save = N_FALSE;
//	}
//	else
//		save = N_FALSE;
//
//	if (!save)
//		mgr->idx[id].block = -1;
//
//	cacheMgr_removeCache(mgr, id);
//}

int cacheMgr_load(NCacheMgr* mgr, const char* uri)
{
	int pos = index_gen(uri);
	uint8 sig[16];
	NCacheItem* item = &mgr->idx[pos];
	NBK_md5(uri, nbk_strlen(uri), sig);
	if (item->time > 0 && index_compare_sig(item->sig, sig)) {
		char path[MAX_CACHE_PATH_LEN];
		cache_genFilePath(path, item->sig);
		if (nbk_pathExist(path))
			return pos;
		else
			return -1;
	}
	else
		return -1;
}

nid cacheMgr_loadDataInfo(NCacheMgr* mgr, int id, int* dataLen, int* userLen)
{
	if (dataLen)
		*dataLen = mgr->idx[id].dataLen;
	if (userLen)
		*userLen = mgr->idx[id].userLen;
	return mgr->idx[id].type;
}

int cacheMgr_loadData(NCacheMgr* mgr, int id, int offset, void* buf, int size)
{
	FILE* fd;
	char path[MAX_CACHE_PATH_LEN];
	int read = 0;
	cache_genFilePath(path, mgr->idx[id].sig);
	fd = fopen(path, "rb");
	if (fd) {
		fseek(fd, offset, SEEK_SET);
		read = fread(buf, 1, size, fd);
		fclose(fd);
	}
	return read;
}
