/*
 * Symbol Table Stack
 *
 * Created on: 2012-11-30
 *     Author: wuyulun
 */

#ifndef __JS_SYMSTACK_H__
#define __JS_SYMSTACK_H__

#include "symTab.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JS_MAX_SYM_STACK_DEPTH	64

typedef struct _JSymStack {
	JSymTab*	sta[JS_MAX_SYM_STACK_DEPTH];
	int			cur;
} JSymStack;

JSymStack* symStack_create(void);
void symStack_delete(JSymStack** sta);

void symStack_enterLocal(JSymStack* sta, const char* name, JValue* value);
nbool symStack_lookupLocal(JSymStack* sta, const char* name, JValue** value);
nbool symStack_lookup(JSymStack* sta, const char* name, JValue** value);

#ifdef __cplusplus
}
#endif

#endif // __JS_SYMSTACK_H__
