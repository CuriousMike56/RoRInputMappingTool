#ifndef MEMORY_WRAPPER_H__
#define MEMORY_WRAPPER_H__

#include <stddef.h>   /* for size_t */

#define ror_malloc(x)   MemoryWrapper::allocBytes(x)
#define ror_calloc(x,y) MemoryWrapper::callocBytes(x,y)
#define ror_free(x)     MemoryWrapper::deallocBytes(x)

class MemoryWrapper
{
protected:
	static unsigned long memory_used;
public:
	static void* allocBytes(size_t count);
	static void* callocBytes(size_t no, size_t size);
	static void deallocBytes(void* ptr);
	static void* allocBytesAligned(size_t align, size_t count);
	static void deallocBytesAligned(size_t align, void* ptr);
	static unsigned long getMemoryAllocated();
};

#endif //MEMORY_WRAPPER_H__