/*
 * DXObjectBase.h
 *
 *  Created on: 2010-2-20
 *      Author: CoCoMo
 */

#ifndef DXOBJECTBASE_H_
#define DXOBJECTBASE_H_

#include <e32base.h>
#include "DXMemoryManager.h"

namespace DXBrowser
	{
	class DXObjectBase : public CBase
		{
	public:
		inline DXObjectBase() {}
		virtual ~DXObjectBase() {}
		inline TAny* operator new(TUint aSize);
		inline TAny* operator new(TUint aSize, TLeave);
		inline TAny* operator new [](TUint aSize);
		inline TAny* operator new [](TUint aSize, TLeave);

		inline void operator delete(TAny* aPtr);
		inline void operator delete [](TAny* aPtr);
		};
	
	//------------------------------------------------------------------------------
	// CObjectBase::operator new
	//------------------------------------------------------------------------------
	inline TAny* DXObjectBase::operator new(TUint aSize)
		{
		TAny* p = DXMemoryManager::Alloc(aSize);
		if (p)
			Mem::FillZ(p, aSize);
		return p;
		}

	//------------------------------------------------------------------------------
	// CObjectBase::operator new Leave
	//------------------------------------------------------------------------------
	inline TAny* DXObjectBase::operator new(TUint aSize, TLeave)
		{
		TAny* p = DXMemoryManager::AllocL(aSize);
		if (p)
			Mem::FillZ(p, aSize);
		return p;
		}

	//------------------------------------------------------------------------------
	// CObjectBase::operator new []
	//------------------------------------------------------------------------------
	inline TAny* DXObjectBase::operator new [] (TUint aSize)
		{
		TAny* p = DXMemoryManager::Alloc(aSize);
		if (p)
			Mem::FillZ(p, aSize);
		return p;
		}

	//------------------------------------------------------------------------------
	// CObjectBase::operator new [] Leave
	//------------------------------------------------------------------------------
	inline TAny* DXObjectBase::operator new [] (TUint aSize, TLeave)
		{
		TAny* p = DXMemoryManager::AllocL(aSize);
		if (p)
			Mem::FillZ(p, aSize);
		return p;
		}

	//------------------------------------------------------------------------------
	// CObjectBase::operator delete
	//------------------------------------------------------------------------------
	inline void DXObjectBase::operator delete(TAny* aPtr)
		{
		return DXMemoryManager::Free(aPtr);
		}

	//------------------------------------------------------------------------------
	// CObjectBase::operator delete []
	//------------------------------------------------------------------------------
	inline void DXObjectBase::operator delete [] (TAny* aPtr)
		{
		return DXMemoryManager::Free(aPtr);
		}
	}

#endif /* DXOBJECTBASE_H_ */
