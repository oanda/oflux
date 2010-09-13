#ifndef OFLUX_ALLOCATOR
#define OFLUX_ALLOCATOR

/**
 * @file OFluxAllocator.h
 * @author Mark Pichora
 * Holds the interface for allocators used in key parts of the runtime.
 * an implementation for the standard new/delete is there too
 */

#include <new>

namespace oflux {

template<const size_t size>
class AllocatorImplementation {
public:
	virtual ~AllocatorImplementation() {}
	virtual void * get() = 0;
	virtual void put(void *) = 0;
};

/**
 * @class MallocAllocatorImplementation
 * @brief implementation which just defers to the standard malloc/free stuff
 */

template<const size_t size>
class MallocAllocatorImplementation : public AllocatorImplementation<size> {
public:
	MallocAllocatorImplementation() {}
	virtual ~MallocAllocatorImplementation() {}
	virtual void * get() { return malloc(size); }
	virtual void put(void * o) { free(o); }
};


/**
 * @class Allocator
 * @brief holder of the implementation.  Also does the construction and
 * destruction via pattern new etc.
 */

template<typename T>
class Allocator {
public:
	static MallocAllocatorImplementation<sizeof(T)> default_allocator;

	Allocator(AllocatorImplementation<sizeof(T)> * impl = &default_allocator)
		: _impl(impl)
	{}
	inline AllocatorImplementation<sizeof(T)> * reset(
		AllocatorImplementation<sizeof(T)> * new_allocator)
	{
		AllocatorImplementation<sizeof(T)> * old = _impl;
		_impl = new_allocator;
		return old;
	}
	inline void put(T *t) 
	{
		if(t) {
			t->~T();
			_impl->put(t); 
		}
	}
	inline T * get() 
	{ return new (_impl->get()) T(); } // pattern new
	template<typename R1>
	inline T * get(R1 r1)
	{ return new (_impl->get()) T(r1); } // pattern new
	template<typename R1, typename R2>
	inline T * get(R1 r1,R2 r2)
	{ return new (_impl->get()) T(r1,r2); } // pattern new
	template<typename R1, typename R2, typename R3>
	inline T * get(R1 r1,R2 r2,R3 r3)
	{ return new (_impl->get()) T(r1,r2,r3); } // pattern new
private:
	AllocatorImplementation<sizeof(T)> * _impl;
};

template<typename T>
MallocAllocatorImplementation<sizeof(T)>
Allocator<T>::default_allocator;


} // namespace oflux

#endif // OFLUX_ALLOCATOR
