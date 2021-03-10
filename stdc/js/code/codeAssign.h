/*
 * Assignment
 *
 * Created on: 2012-11-30
 *     Author: wuyulun
 */

#ifndef __JS_CODEASSIGN_H__
#define __JS_CODEASSIGN_H__

#include "../../inc/config.h"
#include "../code.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JCodeAssign {
	JCode	d;
} JCodeAssign;

JCodeAssign* codeAssign_create(void);
void codeAssign_delete(JCodeAssign** code);

void codeAssign_parse(JCode* self, JToken* token, void* user);

#ifdef __cplusplus
}
#endif

#endif // __JS_CODEASSIGN_H__
