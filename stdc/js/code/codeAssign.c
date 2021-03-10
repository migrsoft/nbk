#include "codeAssign.h"

JCodeAssign* codeAssign_create(void)
{
	JCodeAssign* n = (JCodeAssign*)NBK_malloc0(sizeof(JCodeAssign));
	n->d.type = CODE_ASSIGN;
	n->d.parse = codeAssign_parse;
	return n;
}

void codeAssign_delete(JCodeAssign** code)
{
	JCodeAssign* n = *code;
	*code = N_NULL;
	NBK_free(n);
}

void codeAssign_parse(JCode* self, JToken* token, void* user)
{
}
