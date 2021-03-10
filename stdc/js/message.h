// JMessage 消息定义

#ifndef __JS_MESSAGE_H__
#define __JS_MESSAGE_H__

#include "../inc/config.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _JEMsgType {
	JSMSG_NONE,
	JSMSG_CODE,
    JSMSG_TOKEN,
	JSMSG_SYNTAX_ERROR,
	JSMSG_LAST
} JEMsgType;

typedef struct _JMsgCode {
	int		lineNo;
	char*	code;
	int		length;
} JMsgCode;

typedef struct _JMsgSyntaxErr {
	int		lineNo;
	int		col;
} JMsgSyntaxErr;

typedef struct _JMessage {
	JEMsgType	type;
	union {
		JMsgCode		code;
        JToken*         token;
		JMsgSyntaxErr	synErr;
	} d;
} JMessage;

#ifdef __cplusplus
}
#endif

#endif // __JS_MESSAGE_H__
