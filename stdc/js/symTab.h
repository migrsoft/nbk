/*
 * Symbol Table
 *
 * Created on: 2012-11-30
 *     Author: wuyulun
 */

#ifndef __JS_SYMTAB_H__
#define __JS_SYMTAB_H__

#include "../tools/hashMap.h"
#include "../tools/strPool.h"
#include "value.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JSymTab {
	NHashMap*	map;
	NStrPool*	pool;
} JSymTab;

JSymTab* symTab_create(void);
void symTab_delete(JSymTab** tab);

void symTab_enter(JSymTab* tab, const char* name, JValue* value);
nbool symTab_lookup(JSymTab* tab, const char* name, JValue** value);

#ifdef __cplusplus
}
#endif

#endif // __JS_SYMTAB_H__
