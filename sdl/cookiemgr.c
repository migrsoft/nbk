// create by wuyulun, 2012.1.19

#include "../stdc/tools/str.h"
#include <stdio.h>
#include "cookiemgr.h"

#define MAX_COOKIE_CAPACITY		128
#define COOKIE_FILE_ID			"NBK-Cookie-001"
#define COOKIE_GROW				2048

enum NEStage {
	NESTAGE_NONE,
	NESTAGE_NAME,
	NESTAGE_VALUE,
	NESTAGE_EXPIRES,
	NESTAGE_PATH,
	NESTAGE_DOMAIN,
	NESTAGE_LRU
};

static void cookie_delete(NCookie** cookie)
{
	NCookie* ck = *cookie;
	if (ck->host)
		NBK_free(ck->host);
	if (ck->name)
		NBK_free(ck->name);
	if (ck->value)
		NBK_free(ck->value);
	if (ck->path)
		NBK_free(ck->path);
	NBK_free(ck);
	*cookie = N_NULL;
}

static NCookie* cookie_create(const char* host, char* cookie)
{
	NCookie* ck = (NCookie*)NBK_malloc0(sizeof(NCookie));
	char *p, *q;
	nid stage = NESTAGE_NAME;

	p = cookie;
	while (1) {

		while (*p && *p == ' ') p++;
		if (*p == 0) break;
		q = p;

		// NAME
		while (*p && *p != '=') p++;
		if (*p == 0 || p - q < 1) break;

		if (stage == NESTAGE_NAME) {
			stage = NESTAGE_VALUE;
			ck->name = (char*)NBK_malloc(p - q + 1);
			nbk_strncpy(ck->name, q, p - q);
		}
		else {
			if (nbk_strncmp(q, "expires", 7) == 0)
				stage = NESTAGE_EXPIRES;
			else if (nbk_strncmp(q, "path", 4) == 0)
				stage = NESTAGE_PATH;
			else if (nbk_strncmp(q, "domain", 6) == 0)
				stage = NESTAGE_DOMAIN;
			else
				stage = NESTAGE_NONE;
		}

		// VALUE
		q = ++p;
		while (*p && *p != ';') p++;
		if (p - q < 1) break;

		if (stage == NESTAGE_VALUE) {
			ck->value = (char*)NBK_malloc(p - q + 1);
			nbk_strncpy(ck->value, q, p - q);
		}
		else if (stage == NESTAGE_EXPIRES) {
		}
		else if (stage == NESTAGE_PATH) {
			ck->path = (char*)NBK_malloc(p - q + 1);
			nbk_strncpy(ck->path, q, p - q);
		}
		else if (stage == NESTAGE_DOMAIN) {
			ck->host = (char*)NBK_malloc(p - q + 1);
			nbk_strncpy(ck->host, q, p - q);
		}

		if (*p) p++;
	}

	// cookie
	if (ck->name == N_NULL) {
		cookie_delete(&ck);
	}
	else if (ck->host == N_NULL) {
		ck->host = (char*)NBK_malloc(nbk_strlen(host) + 1);
		nbk_strcpy(ck->host, host);
	}

	return ck;
}

NCookieMgr* cookieMgr_create(void)
{
	NCookieMgr* mgr = (NCookieMgr*)NBK_malloc0(sizeof(NCookieMgr));
	mgr->lst = (NCookie**)NBK_malloc0(sizeof(NCookie*) * MAX_COOKIE_CAPACITY);
	return mgr;
}

void cookieMgr_delete(NCookieMgr** mgr)
{
	NCookieMgr* m = *mgr;
	int i;

	for (i=0; i < MAX_COOKIE_CAPACITY; i++) {
		if (m->lst[i])
			cookie_delete(&m->lst[i]);
	}
	NBK_free(m->lst);

	NBK_free(m);
	*mgr = N_NULL;
}

static int cm_find_same_cookie(NCookieMgr* mgr, NCookie* ck)
{
	int i;
	NCookie* c;

	for (i=0; i < MAX_COOKIE_CAPACITY; i++) {
		c = mgr->lst[i];
		if (c) {
			if (   nbk_strcmp(c->host, ck->host) == 0
				&& nbk_strcmp(c->name, ck->name) == 0 )
				return i;
		}
	}

	return -1;
}

static int cm_find_insert_pos(NCookieMgr* mgr)
{
	int i;
	NCookie* c;
	int find = -1, lru = 8888888;

	for (i=0; i < MAX_COOKIE_CAPACITY; i++) {
		c = mgr->lst[i];
		if (c == N_NULL)
			return i;
		if (c->lru < lru) {
			find = i;
			lru = c->lru;
		}
	}

	return find;
}

// 保存cookie
void cookieMgr_setCookie(NCookieMgr* mgr, const char* host, char* cookie)
{
	NCookie* ck = cookie_create(host, cookie);
	int pos;

	if (ck == N_NULL)
		return;

	pos = cm_find_same_cookie(mgr, ck);

	if (pos == -1) {
        if (ck->value && nbk_strcmp(ck->value, "deleted")) {
			pos = cm_find_insert_pos(mgr);
			if (mgr->lst[pos])
				cookie_delete(&mgr->lst[pos]);
			mgr->lst[pos] = ck;
			ck = N_NULL;
		}
	}
	else {
		cookie_delete(&mgr->lst[pos]);
		if (ck->value && nbk_strcmp(ck->value, "deleted")) {
			mgr->lst[pos] = ck;
			ck = N_NULL;
		}
	}

	if (ck)
		cookie_delete(&ck);
}

// 获取cookie
char* cookieMgr_getCookie(NCookieMgr* mgr, const char* host)
{
	char* cookie = (char*)NBK_malloc(COOKIE_GROW);
	int i;
	NCookie* ck;
	nbool found, resize;
	int max = COOKIE_GROW;
	int v1, v2, size;
	int p = 0;

	for (i=0; i < MAX_COOKIE_CAPACITY; i++) {
		ck = mgr->lst[i];
		if (ck) {
			found = N_FALSE;
			if (ck->host[0] == '.') {
				v1 = nbk_strlen(host) - 1;
				v2 = nbk_strlen(ck->host) - 1;
				while (v2 >= 0 && v1 >= 0) {
					if (ck->host[v2] != host[v1])
						break;
					v2--;
					v1--;
				}
				if (v2 < 0)
					found = N_TRUE;
			}
			else {
				if (nbk_strcmp(ck->host, host) == 0)
					found = N_TRUE;
			}

			if (found) {
				ck->lru++;
				v1 = nbk_strlen(ck->name);
				v2 = nbk_strlen(ck->value);
				size = v1 + 1 + v2 + 3; // NAME=VALUE;

				resize = N_FALSE;
				while (max - p < size) {
					max += COOKIE_GROW;
					resize = N_TRUE;
				}
				if (resize)
					cookie = (char*)NBK_realloc(cookie, max);

				size = sprintf(&cookie[p], "%s=%s; ", ck->name, ck->value);
				p += size;
			}
		}
	}

	if (p == 0) {
		NBK_free(cookie);
		cookie = N_NULL;
	}

	return cookie;
}

// 载入cookie文件
void cookieMgr_load(NCookieMgr* mgr, const char* fname)
{
	FILE* fd;
	int size, len;
	char* buf;
	char *p, *q;
	int i = 0;
	NCookie* ck = N_NULL;
	nid stage;

	fd = fopen(fname, "rb");
	if (fd == N_NULL)
		return;

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	len = nbk_strlen(COOKIE_FILE_ID);
	if (size > len + 1) {
		fseek(fd, 0, SEEK_SET);
		buf = (char*)NBK_malloc(size + 1);
		size = fread(buf, 1, size, fd);
		buf[size] = 0;

		if (nbk_strncmp(buf, COOKIE_FILE_ID, len) == 0) {
			p = buf + len + 1;
			stage = NESTAGE_DOMAIN;
			while (*p) {
				q = p;
				while (*p != '\t' && *p != '\n') p++;

				if (*p == '\n') {
					if (ck) {
						mgr->lst[i++] = ck;
						ck = N_NULL;
						stage = NESTAGE_DOMAIN;
						if (i == MAX_COOKIE_CAPACITY)
							break;
						p++;
						continue;
					}
					else
						break;
				}

				if (ck == N_NULL)
					ck = (NCookie*)NBK_malloc0(sizeof(NCookie));

				if (stage == NESTAGE_DOMAIN) {
					ck->host = (char*)NBK_malloc(p - q + 1);
					nbk_strncpy(ck->host, q, p - q);
					stage = NESTAGE_NAME;
				}
				else if (stage == NESTAGE_NAME) {
					ck->name = (char*)NBK_malloc(p - q + 1);
					nbk_strncpy(ck->name, q, p - q);
					stage = NESTAGE_VALUE;
				}
				else if (stage == NESTAGE_VALUE) {
					ck->value = (char*)NBK_malloc(p - q + 1);
					nbk_strncpy(ck->value, q, p - q);
					stage = NESTAGE_PATH;
				}
				else if (stage == NESTAGE_PATH) {
					ck->path = (char*)NBK_malloc(p - q + 1);
					nbk_strncpy(ck->path, q , p - q);
					stage = NESTAGE_EXPIRES;
				}
				else if (stage == NESTAGE_EXPIRES) {
					ck->expires = (uint32)NBK_atoi(q);
					stage = NESTAGE_LRU;
				}
				else if (stage == NESTAGE_LRU) {
					ck->lru = NBK_atoi(q);
					stage = NESTAGE_NONE;
				}

				p++;
			}

			if (ck)
				cookie_delete(&ck);
		}

		NBK_free(buf);
	}

	fclose(fd);
}

// 写入cookie文件
void cookieMgr_save(NCookieMgr* mgr, const char* fname)
{
	FILE* fd;
	int i;
	NCookie* ck;
	char cr = '\n';
	char tab = '\t';
	char root = '/';
	char num[32];
	int len;

	fd = fopen(fname, "wb");
	if (fd == N_NULL)
		return;

	fwrite(COOKIE_FILE_ID, 1, nbk_strlen(COOKIE_FILE_ID), fd);
	fwrite(&cr, 1, 1, fd);

	for (i=0; i < MAX_COOKIE_CAPACITY; i++) {
		ck = mgr->lst[i];
		if (ck) {
			fwrite(ck->host, 1, nbk_strlen(ck->host), fd);
			fwrite(&tab, 1, 1, fd);

			fwrite(ck->name, 1, nbk_strlen(ck->name), fd);
			fwrite(&tab, 1, 1, fd);
			fwrite(ck->value, 1, nbk_strlen(ck->value), fd);
			fwrite(&tab, 1, 1, fd);

			if (ck->path)
				fwrite(ck->path, 1, nbk_strlen(ck->path), fd);
			else
				fwrite(&root, 1, 1, fd);
			fwrite(&tab, 1, 1, fd);

			len = sprintf(num, "%u", ck->expires);
			fwrite(num, 1, len, fd);
			fwrite(&tab, 1, 1, fd);

			len = sprintf(num, "%d", ck->lru);
			fwrite(num, 1, len, fd);
			fwrite(&tab, 1, 1, fd);

			fwrite(&cr, 1, 1, fd);
		}
	}
	fclose(fd);
}
