/*
 * DXAllocator.cpp
 *
 *  Created on: 2010-2-24
 *      Author: CoCoMo
 */

#include "DXAllocator.h"
#include "fast_malloc.h"

namespace DXBrowser
	{

	DXAllocator::DXAllocator()
		{
		}

	DXAllocator::~DXAllocator()
		{
		}
	
	//==================================================================
	DXDefaultAllocator::DXDefaultAllocator()
		{
		}

	DXDefaultAllocator::~DXDefaultAllocator()
		{
		}
	
	DXDefaultAllocator* DXDefaultAllocator::NewL(void)
		{
		DXDefaultAllocator* self = DXDefaultAllocator::NewLC();
		CleanupStack::Pop();
		return self;
		}
	
	DXDefaultAllocator* DXDefaultAllocator::NewLC(void)
		{
		DXDefaultAllocator* self = new (ELeave) DXDefaultAllocator;
		CleanupStack::PushL(self);
		return self;
		}
	
    TAny* DXDefaultAllocator::Allocate(TUint aSize)
    	{
    	return User::Alloc(aSize);
    	}
    
    TAny* DXDefaultAllocator::ReAllocate(TAny* aPtr, TUint aSize)
    	{
    	return User::ReAlloc(aPtr, aSize);
    	}
    
    void DXDefaultAllocator::Free(TAny* aPtr)
    	{
    	return User::Free(aPtr);
    	}
    
    TUint DXDefaultAllocator::MemorySize(TAny* aPtr)
    	{
    	return User::AllocLen(aPtr);
    	}
	
	//==================================================================
	DXFastAllocator::DXFastAllocator()
		{
		}

	DXFastAllocator::~DXFastAllocator()
		{
		}
	
	DXFastAllocator* DXFastAllocator::NewL(void)
		{
		DXFastAllocator* self = DXFastAllocator::NewLC();
		CleanupStack::Pop();
		return self;
		}
	
	DXFastAllocator* DXFastAllocator::NewLC(void)
		{
		DXFastAllocator* self = new (ELeave) DXFastAllocator;
		CleanupStack::PushL(self);
		return self;
		}
	
    TAny* DXFastAllocator::Allocate(TUint aSize)
    	{
    	return fast_malloc(aSize);
    	}
    
    TAny* DXFastAllocator::ReAllocate(TAny* aPtr, TUint aSize)
    	{
    	return fast_realloc(aPtr, aSize);
    	}
    
    void DXFastAllocator::Free(TAny* aPtr)
    	{
    	return fast_free(aPtr);
    	}
    
    TUint DXFastAllocator::MemorySize(TAny* aPtr)
    	{
    	return fast_malloc_size(aPtr);
    	}

	}
