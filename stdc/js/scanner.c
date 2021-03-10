#include "../tools/str.h"
#include "scanner.h"
#include "script.h"
#include "js_keywords.h"

#define IS_SPACE(c) (c == ' '  || c == '\t')
#define IS_LETTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IS_DIGIT(c) (c >= '0' && c <= '9')
#define IS_HEX(c) ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define IS_SYMBOL(c) (c >= '!' && c <= '~')
#define IS_COMPOUND(c) (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}')

static char* OPERATOR_L1_STR = "+-*/%=,.()[]{};";
static nid OPERATOR_L1_ID[] = {
	SYMBOL_PLUS,
	SYMBOL_MINUS,
	SYMBOL_MUL,
	SYMBOL_DIV,
	SYMBOL_MOD,
	SYMBOL_EQUALS,
	SYMBOL_COMMA,
	SYMBOL_DOT,
	SYMBOL_LEFT_PAREN,
	SYMBOL_RIGHT_PAREN,
	SYMBOL_LEFT_BRACKET,
	SYMBOL_RIGHT_BRACKET,
	SYMBOL_LEFT_BRACE,
	SYMBOL_RIGHT_BRACE,
	SYMBOL_SEMICOLON
};

static nid checkKeyword(const char** set, nid last, const char* name, int len)
{
	char key[MAX_KEYWORD_LEN + 1];

    if (len > MAX_KEYWORD_LEN)
        return N_INVALID_ID;

	nbk_strncpy(key, name, len);
	return js_getKeywordId(key);
}

static char* scanner_readline(JScanner* scanner)
{
	if (scanner->cur < scanner->s_tail) {
		char* h = scanner->cur;
		char* p = h;
		while (p < scanner->s_tail && *p != '\n')
			p++;
		if (*p == '\n')
			*p++ = 0;
		scanner->cur = p;
		scanner->lineNo++;
		return h;
	}
	else { // 扫描结束
		return N_NULL;
	}
}

static void scanner_extractWord(JScanner* scanner, int col, char* txt, int len)
{
    JToken token;
    nid id;

    id = checkKeyword(js_getKeywordNames(), JKWID_LAST, txt, len);
    token.type = (id != N_INVALID_ID) ? TOKEN_KEYWORD : TOKEN_INDENTIFIER;
    token.row = scanner->lineNo;
    token.col = col;
    token.text = txt;
    token.textLen = len;
	token.id = id;
    scanner->handleToken(&token, scanner->script);
}

static void scanner_extractSymbol(JScanner* scanner, int col, char* txt, int len)
{
    JToken token;

    token.type = TOKEN_SYMBOL;
    token.row = scanner->lineNo;
    token.col = col;
	token.id = SYMBOL_UNKNOWN;
    token.text = txt;
    token.textLen = len;

	switch (len) {
	case 1:
		{
			int r = nbk_strchr(OPERATOR_L1_STR, *txt);
			if (r != -1)
				token.id = OPERATOR_L1_ID[r];
		}
		break;

	case 2:
		break;

	case 3:
		break;

	case 4:
		if (nbk_strcmp(txt, ">>>=") == 0)
			token.id = SYMBOL_SHIFTRZ_EQUALS;
		break;
	}

    scanner->handleToken(&token, scanner->script);
}

static void scanner_extractNumber(JScanner* scanner, int col, char* txt, int len)
{
    JToken token;
	char buf[64];

	nbk_strncpy(buf, txt, len);

    token.type = TOKEN_NUMBER;
    token.row = scanner->lineNo;
    token.col = col;
    token.text = txt;
    token.textLen = len;
	token.n = NBK_atol(buf);
    scanner->handleToken(&token, scanner->script);
}

static void scanner_extractToken(JScanner* scanner, JETokenType type, int col, char* txt, int len)
{
    switch (type) {
    case TOKEN_INDENTIFIER:
        scanner_extractWord(scanner, col, txt, len);
        break;

    case TOKEN_SYMBOL:
        scanner_extractSymbol(scanner, col, txt, len);
        break;

    case TOKEN_NUMBER:
        scanner_extractNumber(scanner, col, txt, len);
        break;
    }
}

static void scanner_extract(JScanner* scanner, char* line)
{
	char* p = line;
	char* t = N_NULL; // 标记
	int tc; // 标记列号
	JETokenType type = TOKEN_UNKNOWN;
	JMessage msg;

	while (*p) {
		if (IS_SPACE(*p)) {
            if (type != TOKEN_UNKNOWN) {
                scanner_extractToken(scanner, type, tc, t, p - t);
            }
            type = TOKEN_UNKNOWN;
		}
		else if (*p == '_' || *p == '$' || IS_LETTER(*p)) {
			if (type == TOKEN_UNKNOWN) {
				t = p;
				tc = p - line;
				type = TOKEN_INDENTIFIER;
			}
            else if (type == TOKEN_SYMBOL) {
                scanner_extractToken(scanner, type, tc, t, p - t);
                t = p;
                tc = p - line;
                type = TOKEN_INDENTIFIER;
            }
            else if (type == TOKEN_NUMBER && IS_HEX(*p)) {
                // 字母作为十六进制一部分，忽略
            }
			else if (type != TOKEN_INDENTIFIER) {
				msg.type = JSMSG_SYNTAX_ERROR;
				msg.d.synErr.lineNo = scanner->lineNo;
				msg.d.synErr.col = p - line;
				script_sendMsg((JScript*)scanner->script, &msg);
				break;
			}
		}
		else if (IS_DIGIT(*p)) {
			if (type == TOKEN_UNKNOWN) {
				t = p;
				tc = p - line;
				type = TOKEN_NUMBER;
			}
            else if (type == TOKEN_INDENTIFIER) {
                // 数字作为描述符一部，忽略
            }
            else if (type != TOKEN_NUMBER) {
                scanner_extractToken(scanner, type, tc, t, p - t);
                t = p;
                tc = p - line;
                type = TOKEN_NUMBER;
            }
		}
        else if (IS_SYMBOL(*p)) {
            if (type == TOKEN_UNKNOWN) {
                t = p;
                tc = p - line;
                type = TOKEN_SYMBOL;
            }
            else if (type == TOKEN_SYMBOL) {
                if (IS_COMPOUND(*p)) {
                    scanner_extractToken(scanner, type, tc, t, p - t);
                    t = p;
                    tc = p - line;
                    type = TOKEN_SYMBOL;
                }
            }
            else {
                scanner_extractToken(scanner, type, tc, t, p - t);
                t = p;
                tc = p - line;
                type = TOKEN_SYMBOL;
            }
        }
		else if (*p == '\'' || *p == '"') { // 字符串
		}
		else {
		}
		p++;
	}

    if (type != TOKEN_UNKNOWN) {
        scanner_extractToken(scanner, type, tc, t, p - t);
    }
}

JScanner* scanner_create(void)
{
	JScanner* s = (JScanner*)NBK_malloc0(sizeof(JScanner));
	return s;
}

void scanner_delete(JScanner** scanner)
{
	JScanner* s = *scanner;
	*scanner = N_NULL;
	NBK_free(s);
}

void scanner_write(JScanner* scanner, char* source, int length)
{
	char* line;
	JMessage msg;

	scanner->source = source;
	scanner->s_tail = source + length;
	scanner->cur = scanner->source;
	scanner->lineNo = 0;

	while ((line = scanner_readline(scanner)) != N_NULL) {
		msg.type = JSMSG_CODE;
		msg.d.code.lineNo = scanner->lineNo;
		msg.d.code.code = line;
		msg.d.code.length = scanner->cur - line;
		script_sendMsg((JScript*)scanner->script, &msg);

        scanner_extract(scanner, line);
	}
}
