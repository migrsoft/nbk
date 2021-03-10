#include "codeVar.h"
#include "../../tools/str.h"

JCodeVar* codeVar_create(void)
{
	JCodeVar* n = (JCodeVar*)NBK_malloc0(sizeof(JCodeVar));
	n->d.type = CODE_VAR;
	n->d.parse = codeVar_parse;
	return n;
}

void codeVar_delete(JCodeVar** code)
{
	JCodeVar* n = *code;
	*code = N_NULL;
	if (n->ident)
		NBK_free(n->ident);
	NBK_free(n);
}

void codeVar_parse(JCode* self, JToken* token, void* user)
{
	JCodeVar* n = (JCodeVar*)self;

	if (token->type != TOKEN_INDENTIFIER)
		return;

	if (n->ident) {
		NBK_free(n->ident);
		n->ident = N_NULL;
	}

	N_ASSERT(token->textLen > 0);

	n->ident = (char*)NBK_malloc(token->textLen + 1);
	nbk_strncpy(n->ident, token->text, token->textLen);
}
