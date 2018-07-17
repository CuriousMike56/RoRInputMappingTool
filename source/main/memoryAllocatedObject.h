#ifndef MEMORYALLOCATEDOBJECT_H__
#define MEMORYALLOCATEDOBJECT_H__

#include "memoryWrapper.h"

class MemoryAllocatedObject
{
public:
	explicit MemoryAllocatedObject()
	{
	}

	~MemoryAllocatedObject()
	{
	}

	void* operator new(size_t sz)
	{
		return MemoryWrapper::allocBytes(sz);
	}

	/// placement operator new
	void* operator new(size_t sz, void* ptr)
	{
		return ptr;
	}

	void* operator new[] ( size_t sz )
	{
		return MemoryWrapper::allocBytes(sz);
	}

	void operator delete( void* ptr )
	{
		MemoryWrapper::deallocBytes(ptr);
	}

	void operator delete( void* ptr, void* )
	{
		MemoryWrapper::deallocBytes(ptr);
	}

	void operator delete[] ( void* ptr )
	{
		MemoryWrapper::deallocBytes(ptr);
	}
};

#endif //MEMORYALLOCATEDOBJECT_H__
