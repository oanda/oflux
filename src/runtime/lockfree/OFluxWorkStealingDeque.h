#ifndef OFLUX_WS_DEQUE
#define OFLUX_WS_DEQUE
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
 * @file OFluxWorkStealingDeque.h
 * @author Mark Pichora
 * This code is based on the paper:
 *  David Chase and Yososi Lev
 *  "Dynamic Circular Work-Stealing Deques" (SPAA'05)
 * It implements a deque on which one end can be locally accessed within the
 * thread, and the other end can be used by other threads for stealing 
 * elements.  The cost of stealling involves a CAS, and the use of the local
 */

#include <cstdio>
#include <strings.h>
#include "lockfree/OFluxThreadNumber.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "lockfree/OFluxSentinel.h"

namespace oflux {
namespace lockfree {


template<typename T>
class CircularArray {
public:
	typedef T * TPtr;
	enum { default_log_size = 8 };
	CircularArray(int log_size = default_log_size)
		: _log_size(log_size)
		, _old(NULL)
	{
		_data = new TPtr[1<< log_size];
	}
	~CircularArray()
	{
		delete [] _data;
		delete _old;
	}
	inline long size() const
	{ return 1 << _log_size; }
	inline T * get(long i) const
	{ return _data[i % size()]; }
	inline void put(long i,T * o)
	{ _data[i % size()] = o; }
	CircularArray<T> * grow(long b, long t) const
	{
		CircularArray<T> * res = 
			new CircularArray<T>(_log_size+1);
		for(long i = t; i < b; ++i) {
			res->put(i,get(i));
		}
		res->_old = this;
		return res;
	}
private: 
	int _log_size;
	const CircularArray<T> * _old;
	T ** _data;
	
};

template<typename T>
class CircularWorkStealingDeque {
private:
	static CheapSentinel<T> empty_sentinel;
	static CheapSentinel<T> abort_sentinel;
public: 
	// sentinels: 
	static T * empty;
	static T * abort;

#ifdef CSW_DEQUE_BIG_LOG
	size_t big_log[100000];
#endif

	CircularWorkStealingDeque()
		: _bottom(0)
		, _top(0)
		, _active_array(new CircularArray<T>())
	{
#ifdef CSW_DEQUE_BIG_LOG
	::bzero(big_log,100000);
#endif
	}

	~CircularWorkStealingDeque()
	{ 
		delete _active_array; 
	}

	inline bool cas_top(long oldVal,long newVal)
	{ return __sync_bool_compare_and_swap(&_top,oldVal,newVal); }

	inline void pushBottom(T * e)
	{
		long b = _bottom;
		long t = _top;
		CircularArray<T> * a = _active_array;
		long size= b-t;
		if(size >= a->size()-1) {
			a = a->grow(b,t);
			_active_array = a;
		}
#ifdef CSW_DEQUE_BIG_LOG
		big_log[b%100000] |= (1 << (_tn.index + 16));
		assert(b < 100000);
#endif
		a->put(b,e);
		write_barrier();
		_bottom = b+1;
	}

	inline T * steal()
	{
		long t = _top;
		load_load_barrier();
		long b = _bottom;
		CircularArray<T> * a = _active_array;
		long size = b - t;
		if(size <= 0) {
			return empty;
		}
		T * e = a->get(t);
		if(!cas_top(t,t+1)) {
			return abort;
		}
#ifdef CSW_DEQUE_BIG_LOG
		big_log[t%100000] |= (1 << _tn.index);
#endif
		return e;
	}

	inline T * popBottom()
	{
		CircularArray<T> * a = _active_array;
		long b = _bottom;
		long t;
		long size;
		b = b-1;
		_bottom = b;
		store_load_barrier();
		t = _top;
		size = b - t;
		if(size < 0) {
			_bottom = t;
			return empty;
		}
		T * e = a->get(b);
		if(size>0) {
#ifdef CSW_DEQUE_BIG_LOG
			big_log[b%100000] |= (1 << (_tn.index+8));
#endif
			return e;
		}
		if(!cas_top(t,t+1)) {
			e = empty;
		}
		_bottom = t+1;
#ifdef CSW_DEQUE_BIG_LOG
		if(e != empty) { big_log[b%100000] |= (1 << (_tn.index +8)); }
#endif
		return e;
	}
	void dump()
	{
		printf("deque at %p:\n",this);
		long t = _top;
		for(long i = t; i < _bottom; ++i) {
			printf(" [%ld] = %p\n",i-t,_active_array.get(i));
		}
	}
	inline long size() const { return _bottom - _top; }

private:
	volatile long _bottom;
	volatile long _top;
	CircularArray<T> * _active_array;
	char _align_dontcare[256 - 2*sizeof(long) - sizeof(CircularArray<T> *)];
};

template< typename T >
CheapSentinel<T> 
CircularWorkStealingDeque<T>::empty_sentinel;

template< typename T >
CheapSentinel<T> 
CircularWorkStealingDeque<T>::abort_sentinel;


template< typename T>
T * CircularWorkStealingDeque<T>::empty = CircularWorkStealingDeque<T>::empty_sentinel();

template< typename T>
T * CircularWorkStealingDeque<T>::abort = CircularWorkStealingDeque<T>::abort_sentinel();

} // namespace lockfree
} // namespace oflux

#endif // OFLUX_WS_DEQUE
