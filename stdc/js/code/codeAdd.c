#include "codeAdd.h"

JCodeAdd* codeAdd_create(void)
{
	JCodeAdd* n = (JCodeAdd*)NBK_malloc0(sizeof(JCodeAdd));
	n->d.type = CODE_ADD;
	n->d.parse = codeAdd_parse;
	return n;
}

void codeAdd_delete(JCodeAdd** code)
{
	JCodeAdd* n = *code;
	*code = N_NULL;
	NBK_free(n);
}

void codeAdd_parse(JCode* self, JToken* token, void* user)
{
}
