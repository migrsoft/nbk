#ifndef __JS_TOKEN_H__
#define __JS_TOKEN_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _JETokenType {
	TOKEN_UNKNOWN,
	TOKEN_KEYWORD,	        // 程序保留字
	TOKEN_INDENTIFIER,		// 标识符
	TOKEN_NUMBER,			// 数字
	TOKEN_SYMBOL,	        // 特殊标记
	TOKEN_STRING,			// 字符串
	TOKEN_LAST
} JETokenType;

typedef struct _JToken {
	JETokenType		type; // 标记类型
    int             row;
    int             col;
    char*           text;
    int             textLen;
	nid				id;
	double			n;
} JToken;

enum JESymbolType {
	SYMBOL_UNKNOWN,
	SYMBOL_PLUS_PLUS,		// ++
	SYMBOL_MINUS_MINUS,		// --
	SYMBOL_PLUS,			// +
	SYMBOL_MINUS,			// -
	SYMBOL_INVERT_BITS,		// ~
	SYMBOL_INVERT_BOOL,		// !
	SYMBOL_MUL,				// *
	SYMBOL_DIV,				// /
	SYMBOL_MOD,				// %
	SYMBOL_SHFIT_LEFT,		// <<
	SYMBOL_SHFIT_RIGHT,		// >>
	SYMBOL_SHFIT_RIGHT_Z,	// >>>
	SYMBOL_LESS_THAN,		// <
	SYMBOL_LESS_EQUALS,		// <=
	SYMBOL_GREATER_THAN,	// >
	SYMBOL_GREATER_EQUALS,	// >=
	SYMBOL_TEST_EQUALS,		// ==
	SYMBOL_TEST_INEQUALS,	// !=
	SYMBOL_STRICT_EQUALS,	// ===
	SYMBOL_STRICT_INEQUALS,	// !==
	SYMBOL_AND,				// &
	SYMBOL_XOR,				// ^
	SYMBOL_OR,				// |
	SYMBOL_LOG_AND,			// &&
	SYMBOL_LOG_OR,			// ||
	SYMBOL_QUEST,			// ?
	SYMBOL_EQUALS,			// =
	SYMBOL_MUL_EQUALS,		// *=
	SYMBOL_DIV_EQUALS,		// /=
	SYMBOL_MOD_EQUALS,		// %=
	SYMBOL_ADD_EQUALS,		// +=
	SYMBOL_DES_EQUALS,		// -=
	SYMBOL_AND_EQUALS,		// &=
	SYMBOL_XOR_EQUALS,		// ^=
	SYMBOL_OR_EQUALS,		// |=
	SYMBOL_SHIFTL_EQUALS,	// <<=
	SYMBOL_SHIFTR_EQUALS,	// >>=
	SYMBOL_SHIFTRZ_EQUALS,	// >>>=
	SYMBOL_COMMA,			// ,
	SYMBOL_DOT,				// .
	SYMBOL_LEFT_PAREN,		// (
	SYMBOL_RIGHT_PAREN,		// )
	SYMBOL_LEFT_BRACKET,	// [
	SYMBOL_RIGHT_BRACKET,	// ]
	SYMBOL_LEFT_BRACE,		// {
	SYMBOL_RIGHT_BRACE,		// }
	SYMBOL_SEMICOLON,		// ;
	SYMBOL_LAST
};

#ifdef __cplusplus
}
#endif

#endif // __JS_TOKEN_H__
