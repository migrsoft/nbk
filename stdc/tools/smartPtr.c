#include "smartPtr.h"

void smartptr_init(void* ptr, SP_FREE_FUNC func)
{
	NSmartPtr* p = (NSmartPtr*)ptr;
	p->ref = 1;
	p->free = func;
}

void smartptr_get(void* ptr)
{
	NSmartPtr* p = (NSmartPtr*)ptr;
	p->ref++;
}

void smartptr_release(void* ptr)
{
	NSmartPtr* p = (NSmartPtr*)ptr;
	p->ref--;
	if (p->ref <= 0)
		p->free(&ptr);
}
