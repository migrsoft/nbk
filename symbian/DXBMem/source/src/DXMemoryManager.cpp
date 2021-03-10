/*
 * DXMemoryManager.cpp
 *
 *  Created on: 2010-2-25
 *      Author: CoCoMo
 */

#include "DXMemoryManager.h"
#ifdef EKA2
#include "DXAllocator.h"
#endif

namespace DXBrowser
	{

#ifdef EKA2
	static DXAllocator* _sAllocator = NULL;

	inline DXAllocator* Allocator()
		{
		if (!_sAllocator)
			{
#ifdef __USE_FAST_MALLOC__
			_sAllocator = DXFastAllocator::NewL();
#else
			_sAllocator = DXDefaultAllocator::NewL();
#endif
			}
		return _sAllocator;
		}
#endif

	DXMemoryManager::DXMemoryManager()
		{
		}

	DXMemoryManager::~DXMemoryManager()
		{
		}
	
	EXPORT_C TAny* DXMemoryManager::Alloc(TUint aSize)
		{
#ifdef __USE_FAST_MALLOC__
		return Allocator()->Allocate(aSize);
#else
		return User::Alloc(aSize);
#endif
		}
	
	EXPORT_C TAny* DXMemoryManager::AllocL(TUint aSize)
		{
#ifdef __USE_FAST_MALLOC__
		TAny* p = Allocator()->Allocate(aSize);
		if (!p)
			User::LeaveNoMemory();
		return p;
#else
		return User::AllocL(aSize);
#endif
		}
	
	EXPORT_C TAny* DXMemoryManager::ReAlloc(TAny* aPtr, TUint aSize)
		{
#ifdef __USE_FAST_MALLOC__
		return Allocator()->ReAllocate(aPtr, aSize);
#else
		return User::ReAlloc(aPtr, aSize);
#endif
		}
        
	EXPORT_C TAny* DXMemoryManager::ReAllocL(TAny* aPtr, TUint aSize)
		{
#ifdef __USE_FAST_MALLOC__
		TAny* p = Allocator()->ReAllocate(aPtr, aSize);
		if (!p)
			User::LeaveNoMemory();
		return p;
#else
		return User::ReAllocL(aPtr, aSize);
#endif
		}
	
	EXPORT_C void DXMemoryManager::Free(TAny* aPtr)
		{
#ifdef __USE_FAST_MALLOC__
		return Allocator()->Free(aPtr);
#else
		return User::Free(aPtr);
#endif
		}
	
	EXPORT_C TUint DXMemoryManager::MemorySize(TAny* aPtr)
    	{
#ifdef __USE_FAST_MALLOC__
    	return Allocator()->MemorySize(aPtr);
#else
    	return User::AllocLen(aPtr);
#endif
    	}
    
	EXPORT_C void DXMemoryManager::Exit(void)
    	{
#ifdef EKA2
    	delete _sAllocator;
    	_sAllocator = NULL;
#endif
    	}

	}
