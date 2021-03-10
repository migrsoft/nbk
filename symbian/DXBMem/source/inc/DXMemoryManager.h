/*
 * DXMemoryManager.h
 *
 *  Created on: 2010-2-25
 *      Author: CoCoMo
 */

#ifndef DXMEMORYMANAGER_H_
#define DXMEMORYMANAGER_H_

#include <e32base.h>

#if (!defined _DEBUG && defined EKA2)
#define __USE_FAST_MALLOC__
#endif

namespace DXBrowser
	{

	class DXMemoryManager : public CBase
		{
	public:
		virtual ~DXMemoryManager();
		
	protected:
		DXMemoryManager();
		
	public:
		IMPORT_C static TAny* Alloc(TUint aSize);
		
		IMPORT_C static TAny* AllocL(TUint aSize);
		
		IMPORT_C static TAny* ReAlloc(TAny* aPtr, TUint aSize);
		
		IMPORT_C static TAny* ReAllocL(TAny* aPtr, TUint aSize);
		
		IMPORT_C static void Free(TAny* aPtr);
		
		IMPORT_C static TUint MemorySize(TAny* aPtr);
	    
		IMPORT_C static void Exit(void);
		};

	}

#endif /* DXMEMORYMANAGER_H_ */
