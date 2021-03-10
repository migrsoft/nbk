/*
 * Script
 *
 * Created on: 2012-11-22
 *     Author: wuyulun
 */

#ifndef __JS_SCRIPT_H__
#define __JS_SCRIPT_H__

#include "../inc/config.h"
#include "jsconf.h"
#include "message.h"
#include "scanner.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*JMSG_LISTENER)(JMessage* msg);

typedef struct _JScript {
	int32		ver;

	JScanner*	scanner;

	// 消息侦听
	JMSG_LISTENER	msg_listener;

} JScript;

// 创建、销毁
JScript* script_create(void);
void script_delete(JScript** script);

// 消息相关
void script_setMsgListener(JScript* script, JMSG_LISTENER listener);
void script_sendMsg(JScript* script, JMessage* msg);

void script_execute(JScript* script, char* source, int length);

#ifdef __cplusplus
};
#endif

#endif // __JS_SCRIPT_H__
