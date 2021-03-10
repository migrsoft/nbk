/*
 * Code Base
 *
 * Created on: 2012-11-26
 *     Author: wuyulun
 */

#ifndef __JS_CODE_H__
#define __JS_CODE_H__

#include "../inc/config.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JCode JCode;

enum JECodeType {
	CODE_UNKNOWN,
    CODE_NODE,
	CODE_VAR,
	CODE_ASSIGN,
	CODE_ADD,
    CODE_LAST
};

typedef struct _JCode {
	nid			type;

	JCode*		parent;
	JCode*		child;
	JCode*		prev;
	JCode*		next;

    void (*parse)(JCode* self, JToken* token, void* user);
} JCode;

#ifdef __cplusplus
}
#endif

#endif // __JS_CODE_H__
