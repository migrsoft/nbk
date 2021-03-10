#include "hashMap.h"
#include "str.h"
#include "dump.h"

#define HASHMAP_DEFAULT_CAPACITY	256

#define MAX_DUMP_LEN	32

static uint32 hash(const char* key, int space)
{
	uint32 h = 5381;
	while (*key) {
		h = ((h << 5) + h) + (uint8)*key; // djb2 algorithms
		key++;
	}
	return h % space;
}

NHashMap* hashMap_create(int capacity)
{
	NHashMap* m = (NHashMap*)NBK_malloc0(sizeof(NHashMap));
	m->size = (capacity <= 0) ? HASHMAP_DEFAULT_CAPACITY : capacity;
	m->lst = (NHashEntry*)NBK_malloc0(sizeof(NHashEntry) * m->size);
	return m;
}

void hashMap_delete(NHashMap** map)
{
	NHashMap* m = *map;
	*map = N_NULL;
	hashMap_clear(m);
	NBK_free(m->lst);
	NBK_free(m);
}

void* hashMap_put(NHashMap* map, const char* key, void* value)
{
	uint32 h = hash(key, map->size);
	void* ret = N_NULL;

	if (map->lst[h].key == N_NULL) {
		map->lst[h].key = (char*)key;
		map->lst[h].value = value;
	}
	else {
		// 追加到冲突链表的尾部
		NHashEntry* p = &map->lst[h];
		if (nbk_strcmp(p->key, key) == 0) { // 更新固定表中的值
			ret = p->value;
			p->key = (char*)key;
			p->value = value;
		}
		else {
			NHashEntry* ent;
			NHashEntry* n = (NHashEntry*)p->next;
			while (n) {
				if (nbk_strcmp(n->key, key) == 0) { // 更新冲突表中已有值
					ret = n->value;
					n->key = (char*)key;
					n->value = value;
					return ret;
				}
				p = n;
				n = (NHashEntry*)n->next;
			}
			ent = (NHashEntry*)NBK_malloc0(sizeof(NHashEntry));
			ent->key = (char*)key;
			ent->value = value;
			p->next = ent;
		}
	}
	return ret;
}

nbool hashMap_get(NHashMap* map, const char* key, void** value)
{
	uint32 h = hash(key, map->size);

	if (map->lst[h].key == N_NULL) {
		// 不存在
	}
	else {
		if (nbk_strcmp(map->lst[h].key, key) == 0) { // 匹配第一个
			*value = map->lst[h].value;
			return N_TRUE;
		}
		else { // 搜索冲突表
			NHashEntry* p = (NHashEntry*)map->lst[h].next;
			while (p) {
				if (nbk_strcmp(p->key, key) == 0) {
					*value = p->value;
					return N_TRUE;
				}
				p = (NHashEntry*)p->next;
			}
		}
	}
	return N_FALSE;
}

void* hashMap_remove(NHashMap* map, const char* key)
{
	uint32 h = hash(key, map->size);
	void* ret = N_NULL;

	if (map->lst[h].key == N_NULL) {
		// 不存在
	}
	else {
		NHashEntry* p = &map->lst[h];
		NHashEntry* n;
		if (nbk_strcmp(p->key, key) == 0) { // 匹配固定表中第一个
			ret = p->value;
			n = (NHashEntry*)p->next;
			if (n) { // 将冲突表第一项移入固定表后释放
				NBK_memcpy(p, n, sizeof(NHashEntry));
				NBK_free(n);
			}
			else { // 没有冲突表，清空
				p->key = N_NULL;
				p->value = N_NULL;
			}
		}
		else { // 操作冲突表
			n = (NHashEntry*)p->next;
			while (n) {
				if (nbk_strcmp(n->key, key) == 0) { // 找到后，从冲突表中删除
					ret = n->value;
					p->next = n->next;
					NBK_free(n);
					break;
				}
				p = n;
				n = (NHashEntry*)n->next;
			}
		}
	}
	return ret;
}

void hashMap_clear(NHashMap* map)
{
	int i;

	for (i=0; i < map->size; i++) {
		if (map->lst[i].next) {
			NHashEntry* p = (NHashEntry*)map->lst[i].next;
			NHashEntry* n;
			while (p) {
				n = (NHashEntry*)p->next;
				NBK_free(p);
				p = n;
			}
		}
	}
	NBK_memset(map->lst, 0, sizeof(NHashEntry) * map->size);
}

void hashMap_dump(NHashMap* map)
{
	int i;
	NHashEntry* ent;
	int collision = 0, waste = 0;
	char buf[64];
	char* p;
	int len;

	dump_char(g_dp, "===== hashmap dump =====", -1);
	dump_return(g_dp);

	for (i=0; i < map->size; i++) {
		dump_int(g_dp, i);
		p = map->lst[i].key;
		if (p) {
			// 输出 key
			len = nbk_strlen(p);
			if (len > MAX_DUMP_LEN) {
				p = p + len - MAX_DUMP_LEN;
				len = MAX_DUMP_LEN;
			}
			dump_char(g_dp, p, len);

			ent = (NHashEntry*)map->lst[i].next;
			while (ent) {
				// 输出冲突表 key
				p = ent->key;
				len = nbk_strlen(p);
				if (len > MAX_DUMP_LEN) {
					p = p + len - MAX_DUMP_LEN;
					len = MAX_DUMP_LEN;
				}
				dump_char(g_dp, p, len);

				ent = (NHashEntry*)ent->next;
				collision++;
			}
		}
		else
			waste++;
		dump_return(g_dp);
	}

	sprintf(buf, "=== COLLISION: %d  WASTE: %d ===", collision, waste);
	dump_char(g_dp, buf, -1);
	dump_return(g_dp);
}
