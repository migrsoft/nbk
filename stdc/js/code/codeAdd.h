/*
 * Add
 *
 * Created on: 2012-11-30
 *     Author: wuyulun
 */

#ifndef __JS_CODEADD_H__
#define __JS_CODEADD_H__

#include "../../inc/config.h"
#include "../code.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JCodeAdd {
	JCode	d;
} JCodeAdd;

JCodeAdd* codeAdd_create(void);
void codeAdd_delete(JCodeAdd** code);

void codeAdd_parse(JCode* self, JToken* token, void* user);

#ifdef __cplusplus
}
#endif

#endif // __JS_CODEADD_H__
