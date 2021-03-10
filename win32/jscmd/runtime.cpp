#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../stdc/inc/config.h"
#include "../../stdc/tools/str.h"

static int l_mem_alloc = 0;
static int l_mem_free = 0;

void runtime_statistic(void)
{
	fprintf(stdout, "=== MEMORY ============\n");
	fprintf(stdout, "allocated: %d\n", l_mem_alloc);
	//fprintf(stdout, "    freed: %d\n", l_mem_free);
	if (l_mem_alloc > l_mem_free)
	fprintf(stdout, "     LEAK: %d !!!\n", l_mem_alloc - l_mem_free);
}

void* NBK_malloc(size_t size)
{
	l_mem_alloc++;
	return malloc(size);
}

void* NBK_malloc0(size_t size)
{
	l_mem_alloc++;
	return calloc(size, 1);
}

void* NBK_realloc(void* ptr, size_t size)
{
	return realloc(ptr, size);
}

void NBK_free(void* p)
{
	l_mem_free++;
	free(p);
}

void NBK_memcpy(void* dst, void* src, size_t size)
{
	memcpy(dst, src, size);
}

void NBK_memset(void* dst, int8 value, size_t size)
{
	memset(dst, value, size);
}

int NBK_atoi(const char* s)
{
	return atoi(s);
}

double NBK_atol(const char* s)
{
	return atol(s);
}
