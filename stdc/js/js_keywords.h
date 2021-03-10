/* this file is generated automatically */

#ifndef __JS_KEYWORDS_H__
#define __JS_KEYWORDS_H__

#ifdef __cplusplus
extern "C" {
#endif

enum JEKEYWORDS {
	JKWID_BREAK = 1,
	JKWID_CASE = 2,
	JKWID_CATCH = 3,
	JKWID_CONTINUE = 4,
	JKWID_DEBUGGER = 5,
	JKWID_DEFAULT = 6,
	JKWID_DELETE = 7,
	JKWID_DO = 8,
	JKWID_ELSE = 9,
	JKWID_FALSE = 10,
	JKWID_FINALLY = 11,
	JKWID_FOR = 12,
	JKWID_FUNCTION = 13,
	JKWID_IF = 14,
	JKWID_IN = 15,
	JKWID_INSTANCEOF = 16,
	JKWID_NEW = 17,
	JKWID_NULL = 18,
	JKWID_RETURN = 19,
	JKWID_SWITCH = 20,
	JKWID_THIS = 21,
	JKWID_THROW = 22,
	JKWID_TRUE = 23,
	JKWID_TRY = 24,
	JKWID_TYPEOF = 25,
	JKWID_VAR = 26,
	JKWID_VOID = 27,
	JKWID_WHILE = 28,
	JKWID_WITH = 29,
	JKWID_LAST = 30
};

#define MAX_KEYWORD_LEN	10

void js_keywordsInit(void);
void js_keywordsRelease(void);
const char** js_getKeywordNames(void);
nid js_getKeywordId(const char* word);

#ifdef __cplusplus
}
#endif

#endif
