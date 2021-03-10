#include "cachemgr.h"
#include "loader.h"
#include "../tools/str.h"
#include "../tools/dump.h"

#define CACHE_PATH_LEN	64
#define CACHE_INDEX		"cache.idx"
#define CACHE_INDEX_ID	"NBKCACHE"

#define MAX_DUMP_LEN	32

#define MIN_QUERY_TIMES	4

static void entry_dump(NCacheEntry* ent, int i)
{
	char* p = ent->url;
	int len = 0;
	
	if (p) {
		len = nbk_strlen(ent->url);
		if (len > MAX_DUMP_LEN) {
			p = ent->url + len - MAX_DUMP_LEN;
			len = MAX_DUMP_LEN;
		}
	}

	dump_int(g_dp, i + 1);
	dump_char(g_dp, p, len);
	dump_uint(g_dp, ent->type);
	dump_uint(g_dp, ent->encoding);
	dump_uint(g_dp, ent->lru);
	dump_uint(g_dp, ent->time);
	dump_int(g_dp, ent->size);
	dump_return(g_dp);
}

void cacheMgr_dump(NCacheMgr* mgr)
{
	int i;
	NCacheEntry* ent;
	int files = 0;

	dump_char(g_dp, "===== Cache Manager =====", -1);
	dump_return(g_dp);

	for (i = 0; i < mgr->capa; i++) {
		ent = &mgr->tab[i];
		if (ent->size)
			files++;
		entry_dump(ent, i);
	}

	dump_int(g_dp, files);
	dump_char(g_dp, "cache files", -1);
	dump_return(g_dp);
}

static void entry_delete(NCacheEntry* e)
{
	if (e->url)
		NBK_free(e->url);

	if (e->buf)
		strBuf_delete(&e->buf);

	if (e->fd)
		fclose(e->fd);

	NBK_memset(e, 0, sizeof(NCacheEntry));
}

static void entry_delete_with_file(NCacheEntry* e, NHashMap* map, const char* path)
{
	NBK_unlink(path);
	hashMap_remove(map, e->url);
	entry_delete(e);
}

NCacheMgr* cacheMgr_create(int capacity)
{
	NCacheMgr* mgr = (NCacheMgr*)NBK_malloc0(sizeof(NCacheMgr));
	mgr->capa = capacity;
	return mgr;
}

void cacheMgr_delete(NCacheMgr** mgr)
{
	NCacheMgr* m = *mgr;
	int i;

	for (i = 0; i < m->capa; i++) {
		entry_delete(&m->tab[i]);
	}

	NBK_free(m->tab);
	hashMap_delete(&m->map);
	NBK_free(m);
	*mgr = N_NULL;
}

static char* suffix[] = {
	"nul",
	"wmc",
	"htm",
	"htz",
	"css",
	"csz",
	"js",
	"jsz",
	"gif",
	"jpg",
	"png"
};

static void get_data_path(char path[], int id, uint8 type, const char* dir)
{
	sprintf(path, "%sn%04d%s", dir, id + 1, suffix[type]);
}

typedef struct _NClean {
	int		idx;
	uint16	lru;
	uint32	time;
} NClean;

static int cacheMgr_check_capacity(NCacheMgr* mgr)
{
	int i, j;
	int max;
	NClean* clean;
	NCacheEntry* ent;
	char path[CACHE_PATH_LEN];
	nbool remove;

	if (mgr->use < mgr->capa)
		return -1;

	max = mgr->capa / 10; // 每次清理 10% 的空间
	clean = (NClean*)NBK_malloc(sizeof(NClean) * max);
	for (j = 0; j < max; j++) {
		clean[j].idx = -1;
		clean[j].lru = 0xffff;
		clean[j].time = 0xffffffff;
	}

	// 找到最少使用的缓存
	for (i = 0; i < mgr->capa; i++) {
		for (j = 0; j < max; j++) {
			remove = N_FALSE;
			if (mgr->tab[i].lru < clean[j].lru)
				remove = N_TRUE;
			else if (mgr->tab[i].lru == clean[j].lru && mgr->tab[i].time < clean[j].time)
				remove = N_TRUE;

			if (remove) {
				clean[j].idx = i;
				clean[j].lru = mgr->tab[i].lru;
				clean[j].time = mgr->tab[i].time;
				break;
			}
		}
	}

	//dump_char(g_dp, "REMOVE ---", -1);
	//dump_return(g_dp);
	for (j = 0; j < max; j++) {
		ent = &mgr->tab[clean[j].idx];
		//entry_dump(ent, clean[j].idx);
		get_data_path(path, clean[j].idx, ent->type, mgr->path);
		entry_delete_with_file(ent, mgr->map, path);
		mgr->use--;
	}

	i = clean[0].idx;

	NBK_free(clean);

	return i; // 返回可用 ID
}

void cacheMgr_setPath(NCacheMgr* mgr, const char* path)
{
	mgr->path = (char*)path;
}

static void get_index_path(char* path, const char* dir)
{
	sprintf(path, "%s%s", dir, CACHE_INDEX);
}

static void entry_read(NCacheEntry* e, FILE* fd)
{
	int len;

	fread(&e->type, 1, 1, fd);
	fread(&e->encoding, 1, 1, fd);
	fread(&e->lru, 2, 1, fd);
	fread(&e->time, 4, 1, fd);
	fread(&e->size, 4, 1, fd);

	fread(&len, 4, 1, fd);
	if (len) {
		e->url = (char*)NBK_malloc(len + 1);
		fread(e->url, 1, len, fd);
		e->url[len] = 0;
	}
}

static void entry_write(NCacheEntry* e, FILE* fd)
{
	int len = (e->url) ? nbk_strlen(e->url) : 0;

	fwrite(&e->type, 1, 1, fd);
	fwrite(&e->encoding, 1, 1, fd);
	fwrite(&e->lru, 2, 1, fd);
	fwrite(&e->time, 4, 1, fd);
	fwrite(&e->size, 4, 1, fd);

	fwrite(&len, 4, 1, fd);
	if (len)
		fwrite(e->url, 1, len, fd);
}

static void cacheMgr_init(NCacheMgr* mgr, int capacity)
{
	mgr->capa = capacity;
	mgr->tab = (NCacheEntry*)NBK_malloc0(sizeof(NCacheEntry) * capacity);
	mgr->map = hashMap_create(capacity);
}

void cacheMgr_load(NCacheMgr* mgr)
{
	char path[CACHE_PATH_LEN];
	FILE* f;
	int i, total;
	NCacheEntry* ent;

	get_index_path(path, mgr->path);
	f = fopen(path, "rb");
	if (f) {
		char id[12];
		i = nbk_strlen(CACHE_INDEX_ID);
		fread(id, 1, i, f);
		if (nbk_strncmp(CACHE_INDEX_ID, id, i) == 0) {
			fread(&total, 4, 1, f); // 总数
			cacheMgr_init(mgr, total);
			mgr->use = 0;
			for (i = 0; i < total && i < mgr->capa; i++) {
				ent = &mgr->tab[i];
				entry_read(ent, f);
				if (ent->url) {
					ent = (NCacheEntry*)hashMap_put(mgr->map, ent->url, ent);
					N_ASSERT(ent == N_NULL);
					mgr->use++;
				}
			}
		}
		fclose(f);

		//dump_char(g_dp, "LOAD", 4);
		//cacheMgr_dump(mgr);
	}
	else {
		cacheMgr_init(mgr, mgr->capa);
	}
}

void cacheMgr_save(NCacheMgr* mgr)
{
	char path[CACHE_PATH_LEN];
	FILE* f;
	int i;
	NCacheEntry* ent;

	//dump_char(g_dp, "SAVE", 4);
	//cacheMgr_dump(mgr);

	get_index_path(path, mgr->path);
	f = fopen(path, "wb");
	if (f) {
		fwrite(CACHE_INDEX_ID, 1, nbk_strlen(CACHE_INDEX_ID), f);
		fwrite(&mgr->capa, 4, 1, f);

		// 索引表顺序与文件名对应，必须存储所有项目
		for (i = 0; i < mgr->capa; i++) {
			ent = &mgr->tab[i];
			entry_write(ent, f);
		}

		fclose(f);
	}
}

// 存储资源。返回缓存ID，用于后续操作。
int cacheMgr_store(NCacheMgr* mgr, const char* url, uint8 type, uint8 encoding)
{
	NCacheEntry* ent;
	int cacheId;

	if (hashMap_get(mgr->map, url, (void**)&ent)) {
		// 该缓存项已存在，检测是否满足缓存条件
		if (ent->lru < MIN_QUERY_TIMES)
			return -1;

		cacheId = ent - mgr->tab;
		return cacheId;
	}

	// 项目不存在，查找可用项
	cacheId = cacheMgr_check_capacity(mgr);
	if (cacheId == -1) {
		int i;
		for (i = 0; i < mgr->capa; i++) {
			if (mgr->tab[i].url == N_NULL) {
				cacheId = i;
				break;
			}
		}
	}

	if (cacheId == -1)
		return -1;

	ent = &mgr->tab[cacheId];
	ent->url = str_clone(url);
	ent->type = type;
	ent->encoding = encoding;
	ent->time = NBK_currentSeconds();
	ent->size = 0;

	//dump_char(g_dp, "TRACK ---", -1);
	//dump_return(g_dp);
	//entry_dump(ent, cacheId);

	ent = (NCacheEntry*)hashMap_put(mgr->map, ent->url, ent);
	N_ASSERT(ent == N_NULL);

	mgr->use++;

	return -1;
}

// 存储缓存数据
void cacheMgr_putData(NCacheMgr* mgr, int cacheId, char* data, int length, nbool over)
{
	NCacheEntry* ent;

	if (cacheId == -1)
		return;
	
	ent = &mgr->tab[cacheId];

	if (over) { // 数据完成，写入文件
		if (ent->buf) {
			char path[CACHE_PATH_LEN];
			FILE* f;

			ent->size = 0;
			get_data_path(path, cacheId, ent->type, mgr->path);
			f = fopen(path, "wb");
			if (f) {
				nbool begin = N_TRUE;
				char* buf;
				int len;
				while (strBuf_getStr(ent->buf, &buf, &len, begin)) {
					begin = N_FALSE;
					fwrite(buf, 1, len, f);
					ent->size += len;
				}
				fclose(f);

				//dump_char(g_dp, "SAVE ---", -1);
				//dump_return(g_dp);
				//entry_dump(ent, cacheId);
			}
			strBuf_delete(&ent->buf);
		}
	}
	else { // 缓存数据至内存
		if (ent->buf == N_NULL)
			ent->buf = strBuf_create();
		strBuf_appendStr(ent->buf, data, length);
	}
}

// 查询缓存
int cacheMgr_find(NCacheMgr* mgr, const char* url, uint8* type, uint8* encoding, int* length)
{
	int cacheId = -1;
	NCacheEntry* ent;

	if (hashMap_get(mgr->map, url, (void**)&ent)) {
		char path[CACHE_PATH_LEN];
		FILE* f;

		ent->lru++;
		ent->time = NBK_currentSeconds();
		cacheId = ent - mgr->tab;

		if (ent->size == 0)
			return -1;

		// 检测文件存在性
		get_data_path(path, cacheId, ent->type, mgr->path);
		f = fopen(path, "rb");
		if (f) {
			int len;
			fseek(f, 0, SEEK_END);
			len = ftell(f);
			fclose(f);

			if (len != ent->size) { // 文件与索引不一致
				cacheId = -1;
				entry_delete_with_file(ent, mgr->map, path);
				mgr->use--;
				cacheId = -1;
			}
			else {
				*type = ent->type;
				*encoding = ent->encoding;
				*length = ent->size;
			}
		}
		else // 文件不存在
			cacheId = -1;
	}

	return cacheId;
}

// 获取缓存数据
int cacheMgr_getData(NCacheMgr* mgr, int cacheId, char* buf, int size, int from)
{
	NCacheEntry* ent;
	int read;

	if (cacheId == -1)
		return 0;

	ent = &mgr->tab[cacheId];

	if (ent->fd == N_NULL) {
		char path[CACHE_PATH_LEN];
		get_data_path(path, cacheId, ent->type, mgr->path);
		ent->fd = fopen(path, "rb");
		if (ent->fd == N_NULL)
			return -1;
	}

	fseek(ent->fd, from, SEEK_SET);
	read = fread(buf, 1, size, ent->fd);
	if (read <= size && feof(ent->fd)) {
		fclose(ent->fd);
		ent->fd = N_NULL;
	}

	return read;
}
