/*
 * JavaScript Value
 *
 * Created on: 2012-11-29
 *     Author: wuyulun
 */

#ifndef __JS_VALUE_H__
#define __JS_VALUE_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _JEValueType {
	JVT_UNDEFINE,
	JVT_CONSTANT,
	JVT_NUMBER,
	JVT_STRING,
	JVT_FUNCTION,
	JVT_ARRAY,
	JVT_OBJECT,
	JVT_CLASS
} JEValueType;

typedef struct _JValue {
	JEValueType		type;
	union {
		double		n;
	} d;
} JValue;

#ifdef __cplusplus
}
#endif

#endif // __JS_VALUE_H__
