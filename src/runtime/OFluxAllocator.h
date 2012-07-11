#ifndef OFLUX_ALLOCATOR
#define OFLUX_ALLOCATOR
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	virtual void * get() 
	{ 
		void * v = malloc(size); 
		if (!v) throw std::bad_alloc();
		return v;
	}
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
