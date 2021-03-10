#include "symStack.h"

JSymStack* symStack_create(void)
{
	JSymStack* s = (JSymStack*)NBK_malloc0(sizeof(JSymStack));
	s->sta[0] = symTab_create();
	s->cur = 0;
	return s;
}

void symStack_delete(JSymStack** sta)
{
	JSymStack* s = *sta;
	*sta = N_NULL;
	while (s->cur >= 0) {
		symTab_delete(&s->sta[s->cur]);
		s->cur--;
	}
	NBK_free(s);
}

void symStack_enterLocal(JSymStack* sta, const char* name, JValue* value)
{
}

nbool symStack_lookupLocal(JSymStack* sta, const char* name, JValue** value)
{
	return N_FALSE;
}

nbool symStack_lookup(JSymStack* sta, const char* name, JValue** value)
{
	return N_FALSE;
}
