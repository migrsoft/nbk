#include "codeNode.h"
#include "../parser.h"
#include "../js_keywords.h"

JCodeNode* codeNode_create(void)
{
    JCodeNode* n = (JCodeNode*)NBK_malloc0(sizeof(JCodeNode));
    n->d.type = CODE_NODE;
    n->d.parse = codeNode_parse;
    return n;
}

void codeNode_delete(JCodeNode** code)
{
    JCodeNode* n = *code;
    *code = N_NULL;
    NBK_free(n);
}

void codeNode_parse(JCode* self, JToken* token, void* user)
{
	JParser* parser = (JParser*)user;
	JCodeNode* my = (JCodeNode*)self;

	parser->active = self;

	if (token == N_NULL)
		return;

	if (token->type == TOKEN_KEYWORD && token->id == JKWID_VAR) {
		my->varDecl = N_TRUE;
		return;
	}
}
