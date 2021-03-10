/*
 * Program Scanner
 *
 * Created on: 2012-11-22
 *     Author: wuyulun
 */

#ifndef __JS_SCANNER_H__
#define __JS_SCANNER_H__

#include "../inc/config.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JScanner {

	void*	script;

	char*	source;	// 程序源码
	char*	s_tail;	// 源码结束位置
	char*	cur;	// 当前解析位置
	int		lineNo;	// 当前行号
	int		col;	// 当前列号

	void (*handleToken)(JToken* token, void* user);

} JScanner;

JScanner* scanner_create(void);
void scanner_delete(JScanner** scanner);

void scanner_write(JScanner* scanner, char* source, int length);

#ifdef __cplusplus
}
#endif

#endif // __JS_SCANNER_H__
