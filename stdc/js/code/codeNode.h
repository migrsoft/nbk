/*
 * Statement
 *
 * Created on: 2012-11-29
 *     Author: wuyulun
 */

#ifndef __JS_CODENODE_H__
#define __JS_CODENODE_H__

#include "../../inc/config.h"
#include "../code.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JCodeNode {
    JCode   d;
	nbool	varDecl;
} JCodeNode;

JCodeNode* codeNode_create(void);
void codeNode_delete(JCodeNode** code);

void codeNode_parse(JCode* self, JToken* token, void* user);

#ifdef __cplusplus
}
#endif

#endif // __JS_CODENODE_H__
