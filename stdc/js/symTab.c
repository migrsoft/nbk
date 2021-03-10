#include "symTab.h"

#define FUNCTION_VAR_TOTAL	32

JSymTab* symTab_create(void)
{
	JSymTab* t = (JSymTab*)NBK_malloc0(sizeof(JSymTab));
	t->map = hashMap_create(FUNCTION_VAR_TOTAL);
	t->pool = strPool_create();
	return t;
}

void symTab_delete(JSymTab** tab)
{
	JSymTab* t = *tab;
	*tab = N_NULL;
	hashMap_delete(&t->map);
	strPool_delete(&t->pool);
	NBK_free(t);
}

void symTab_enter(JSymTab* tab, const char* name, JValue* value)
{
	JValue* old;
	char* key = strPool_save(tab->pool, name, -1);
	old = (JValue*)hashMap_put(tab->map, key, value);
	if (old) {
	}
}

nbool symTab_lookup(JSymTab* tab, const char* name, JValue** value)
{
	nbool ret = hashMap_get(tab->map, name, (void**)value);
	return ret;
}
