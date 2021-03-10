/*
 * Syntax Parser
 *
 * Created on: 2012-11-26
 *     Author: wuyulun
 */

#ifndef __JS_PARSER_H__
#define __JS_PARSER_H__

#include "../inc/config.h"
#include "token.h"
#include "code.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JParser {
	void*	script;

	JCode*	root; // 语法解析树
	JCode*	active;

} JParser;

JParser* parser_create(void);
void parser_delete(JParser** parser);

void parser_parse(JParser* parser, JToken* token);

JCode* parser_createCode(JParser* parser, JToken* token);

#ifdef __cplusplus
}
#endif

#endif // __JS_PARSER_H__
