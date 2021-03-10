/* this file is generated automatically */

#include "../inc/config.h"
#include "../tools/hashMap.h"
#include "js_keywords.h"

static const char* l_names[] = {
	"",
	"break",
	"case",
	"catch",
	"continue",
	"debugger",
	"default",
	"delete",
	"do",
	"else",
	"false",
	"finally",
	"for",
	"function",
	"if",
	"in",
	"instanceof",
	"new",
	"null",
	"return",
	"switch",
	"this",
	"throw",
	"true",
	"try",
	"typeof",
	"var",
	"void",
	"while",
	"with"
};

static NHashMap* l_map = N_NULL;

void js_keywordsInit(void)
{
    if (l_map == N_NULL) {
        int i;
        l_map = hashMap_create(JKWID_LAST);
        for (i=1; i < JKWID_LAST; i++) {
            hashMap_put(l_map, l_names[i], (void*)i);
        }
    }
}

void js_keywordsRelease(void)
{
    if (l_map)
        hashMap_delete(&l_map);
}

const char** js_getKeywordNames(void)
{
    return (const char**)l_names;
}

nid js_getKeywordId(const char* word)
{
    void* v = 0;
    if (hashMap_get(l_map, word, &v))
        return (nid)v;
    else
        return N_INVALID_ID;
}
