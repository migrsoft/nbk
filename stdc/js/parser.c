#include "parser.h"
#include "code/codeNode.h"
#include "code/codeAssign.h"
#include "code/codeAdd.h"
#include "code/codeVar.h"

JParser* parser_create(void)
{
	JParser* p = (JParser*)NBK_malloc0(sizeof(JParser));
	return p;
}

void parser_delete(JParser** parser)
{
	JParser* p = *parser;
	*parser = N_NULL;
	NBK_free(p);
}

void parser_parse(JParser* parser, JToken* token)
{
    if (parser->root == N_NULL) {
        parser->root = (JCode*)codeNode_create();
		parser->active = parser->root;
    }

	parser->active->parse(parser->active, token, parser);
}

JCode* parser_createCode(JParser* parser, JToken* token)
{
    switch (token->type) {
	case TOKEN_INDENTIFIER:
		{
			JCodeVar* n = codeVar_create();
			codeVar_parse((JCode*)n, token, parser);
			return (JCode*)n;
		}
		break;

    case TOKEN_SYMBOL:
        switch (token->id) {
        case SYMBOL_EQUALS:
			return (JCode*)codeAssign_create();
            break;

        case SYMBOL_PLUS:
			return (JCode*)codeAdd_create();
            break;
        }
        break;
    }
    return N_NULL;
}
