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
class MallocAllocatorImplementation : public AllocatorImplementation {
public:
	MallocAllocatorImplementation() {}
	virtual ~MallocAllocatorImplementation() {}
	virtual void * get() { return malloc(size); }
	virtual void put(void * o) { free(o); }
};

class DefaultDeferFree {
public:
	inline static void defer_put(void *v, AllocatorImplementation & ai)
	{
		ai.put(v); // no deferring happening
	}
};


template< const size_t sz >
inline AllocatorImplementation *
default_allocator()
{
	static MallocAllocatorImplementation<sz> impl;
	AllocatorImplementation * i = &impl;
	return i;
}

/**
 * @class Allocator
 * @brief holder of the implementation.  Also does the construction and
 * destruction via pattern new etc.
 */

template< typename T
	, typename DF = DefaultDeferFree >
class Allocator {
public:

	Allocator(AllocatorImplementation * impl 
			= default_allocator<sizeof(T)>())
		: _impl(impl)
	{}
	inline AllocatorImplementation * reset(
		AllocatorImplementation * new_allocator)
	{
		AllocatorImplementation * old = _impl;
		_impl = new_allocator;
		return old;
	}
	inline void put(T *t) 
	{
		if(t) {
			t->~T();
			DF::defer_put(t,*_impl);
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
	AllocatorImplementation * _impl;
};



} // namespace oflux

#endif // OFLUX_ALLOCATOR
