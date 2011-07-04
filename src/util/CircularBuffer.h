#ifndef FS_CIRCULAR_BUFFER_H
#define FS_CIRCULAR_BUFFER_H

#include <cstdlib>
#include <cassert>

// I have included this code in the hope that it may be useful
// for implementing fast mutual exclusion in some context in a high write-load context
// where ordering is important.


//
// Very simple lock-free structure:
// Given a T which as member ::run2()
//  For every T* tpr you ::submit(tptr) to this buffer
//    each tptr will ::run2() then be deleted
//    mutual exclusion: no two tptr1 and tptr2 will have their ::run2() 
//      calls overlap (kind of the point)
//    co-operation: each thread participating should be ok with
//      being the thread that executes the ::run2()s of others
// Example application:
//  if the ::run2() sends stuff on a shared socket, then it is crucial not
//    to have simultaneous write happening at the same time
//  Serialization can be slow in some cases (FIX protocol e.g.), but needs
//  to be sequenced some how (sequence ids need to be right)
//   run1: advance a sequence id, serialize the FIX message
//   run2: check the sequence id is 'next one', 
//         if so write packet otherwise correct, redo serialization and write
// run1 should take a period of time that is several times what run2 takes
//  on average for this to be useful.

namespace oflux {
namespace executor {

template< typename T
	, void (T::*run1)()
	, void (T::*run2)() >
class CircularBuffer {
public:
	typedef T * TPtr;

	CircularBuffer(const size_t log_size)
		: _in(0)
		, _half_out(0)
		, _out(0)
		, _array( new TPtr[1 << log_size] )
		, _log_size(log_size)
	{
		const size_t sz = 1 << _log_size;
		for(size_t i = 0; i < sz; ++i) {
			_array[i] = NULL;
		}
	}
	~CircularBuffer()
	{
		const size_t sz = 1 << _log_size;
		TPtr tmp = NULL;
		for(size_t i = 0; i < sz; ++i) {
			tmp = _array[i];
			if(tmp && __sync_bool_compare_and_swap( &(_array[i]), tmp, NULL)) {
				delete tmp;
			}
		}
		delete [] _array;
	}

#define store_load_barrier() \
   __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")

	//
	// the buffer takes ownership of the workItem
	// and will:
	//   (a) ->run1() it
	//   (b) ->run2() it
	//   (c) delete it
	//   (d) ensure run2() calls are sequential
	//   (e) no run2() calls overlap
	//   (f) threads co-operate, help each other
	void submit(T * workItem)
	{
		const size_t sz = 1 << _log_size;
		// reserve a spot
		volatile long long in = __sync_fetch_and_add(&_in,1);
		// run1 execution
		workItem->run1();
		// plant your item or spin (when full -- hope not to do that much)
		store_load_barrier();
		while(_array[in%sz] 
			|| (_in-_out) > sz-1
			|| !__sync_bool_compare_and_swap(&(_array[in%sz]), NULL, workItem)) {}
		// if you are at the _out end
		// start slogging through them
		TPtr tmp = NULL;
		while( _out == in 
				&& (tmp = _array[in%sz]) // assign
				&& (in ? !_array[(in+sz-1)%sz] : true)
				&& __sync_bool_compare_and_swap(&_half_out,in,in+1)) {
			// remove from the array
			_array[in%sz] = NULL;
			tmp->run2(); // cannot throw!
			delete tmp;
			tmp = NULL;
			__sync_bool_compare_and_swap(&_out,in,in+1);
			in = in+1;
		}
	}

	size_t size() const
	{
		return (_in - _out);
	}


private:
	volatile long long _in;
	char _dc1[64 - sizeof(long long)]; // don't care -- cache line alignment
	volatile long long _half_out;
	char _dc2[64 - sizeof(long long)]; // don't care -- cache line alignment
	volatile long long _out;
	char _dc3[64 - sizeof(long long)]; // don't care -- cache line alignment
	T ** _array;
	const size_t _log_size;
};


} // namespace executor
} // namespace oflux

#endif // FS_CIRCULAR_BUFFER_H

