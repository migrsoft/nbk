/*
 * Variable
 *
 * Created on: 2012-11-30
 *     Author: wuyulun
 */

#ifndef __JS_CODEVAR_H__
#define __JS_CODEVAR_H__

#include "../../inc/config.h"
#include "../code.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JCodeVar {
	JCode	d;
	char*	ident;
} JCodeVar;

JCodeVar* codeVar_create(void);
void codeVar_delete(JCodeVar** code);

void codeVar_parse(JCode* self, JToken* token, void* user);

#ifdef __cplusplus
}
#endif

#endif // __JS_CODEVAR_H__
