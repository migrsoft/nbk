/*
 * DXAllocator.h
 *
 *  Created on: 2010-2-24
 *      Author: CoCoMo
 */

#ifndef DXALLOCATOR_H_
#define DXALLOCATOR_H_

#include <e32base.h>

namespace DXBrowser
	{

	class DXAllocator : public CBase
		{
	public:
		virtual ~DXAllocator();
		
	protected:
		DXAllocator();
		
	public:
	    virtual TAny* Allocate(TUint aSize) = 0;

	    virtual TAny* ReAllocate(TAny* aPtr, TUint aSize) = 0;

	    virtual void Free(TAny* aPtr) = 0;
	    
	    virtual TUint MemorySize(TAny* aPtr) = 0;
		};
	
	class DXDefaultAllocator : public DXAllocator
		{
	public:
		virtual ~DXDefaultAllocator();
		
		static DXDefaultAllocator* NewL(void);
		static DXDefaultAllocator* NewLC(void);
		
	protected:
		DXDefaultAllocator();
		
	public:
	    TAny* Allocate(TUint aSize);

	    TAny* ReAllocate(TAny* aPtr, TUint aSize);

	    void Free(TAny* aPtr);
	    
	    TUint MemorySize(TAny* aPtr);
		};
	
	class DXFastAllocator : public DXAllocator
		{
	public:
		virtual ~DXFastAllocator();
		
		static DXFastAllocator* NewL(void);
		static DXFastAllocator* NewLC(void);
		
	protected:
		DXFastAllocator();
		
	public:
	    TAny* Allocate(TUint aSize);

	    TAny* ReAllocate(TAny* aPtr, TUint aSize);

	    void Free(TAny* aPtr);
	    
	    TUint MemorySize(TAny* aPtr);
		};

	}

#endif /* DXALLOCATOR_H_ */
