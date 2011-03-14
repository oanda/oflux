#ifndef OFLUX_CIRCULAR_GROWABLE_ARRAY2
#define OFLUX_CIRCULAR_GROWABLE_ARRAY2

#include <cassert>
#include <cstdio>

/**
 * @file OFluxGrowableCircularArray.h
 * @author Mark Pichora
 * Lock-free Circular Array which can grow.  Also a lock-free queue based on
 * on this circular array for its implementation.
 */


namespace oflux {
namespace lockfree {
namespace growable {

template< typename T >
class CheapSentinel { // meant to be a static instance
public:
	CheapSentinel() : _dontcare(0) {}
	T * operator()() 
	{ return static_cast<T *>(reinterpret_cast<void *>(&_dontcare)); }
private:
	int _dontcare;
};

template< typename T >
struct TStructEntry {
	typedef unsigned long index_t;

	T * ptr;
	index_t at;
};

/**
 * @class CircularArrayImplementation
 * @brief Implementation class for a circular array which does not grow.
 *   The data is held in a heap allocated C array.  Support for putting
 *   A new bigger array in front of an older array is here with the
 *   constructor which takes a pointer argument.
 */

template< typename T >
class CircularArrayImplementation {
public:
	enum { default_log_size = 4 };

	static CheapSentinel<T> uninit_sentinel;

	typedef TStructEntry<T> TEntry;
	
	typedef unsigned long index_t;

	typedef T * TPtr;


	CircularArrayImplementation()
		: _log_size(default_log_size)
		, _old(NULL)
	{
		_data = new TEntry[1 << _log_size];
		_data[0].ptr = NULL;
		_data[0].at = -1;
		for(index_t i = 1; i < size(); ++i) {
			_data[i].ptr = NULL;
			_data[i].at = -1;
		}
	}
	CircularArrayImplementation(CircularArrayImplementation<T> *old)
		: _log_size( old->_log_size + 1 )
		, _old(old)
	{
		_data = new TEntry[1 << _log_size];
		_data[0].ptr = uninit_sentinel();
		_data[0].at = -1;
		for(index_t i = 1; i < size(); ++i) {
			_data[i].ptr = _data[0].ptr;
			_data[i].at = -1;
		}
	}
	~CircularArrayImplementation()
        {
                delete [] _data;
                delete _old;
        }
        inline unsigned long size() const
        { return 1 << _log_size; }
        inline TEntry * get(index_t i, bool update = true) const
        {
		index_t i_mod = i % size();
		TEntry * r =  & (_data[i_mod]);
		static T * const u_sentin = uninit_sentinel();
		if( r->ptr == u_sentin ) { // recurse
			assert(_old);
			TEntry * rold = _old->get(i,false);
			//long old_at = _data[i_mod].at;
			if(update) {
				if(rold->at == i) {
					bool cas_res =
						__sync_bool_compare_and_swap(
							& (_data[i_mod].ptr)
							, u_sentin
							, rold->ptr);
					if(cas_res) {
						_data[i_mod].at = i; // bit late
					}
				} else { // was NULL
					bool cas_res =
						__sync_bool_compare_and_swap(
							& (_data[i_mod].ptr)
							, u_sentin
							, NULL);
					if(!cas_res) {
						assert(r->ptr != u_sentin);
					}
				}
			} else {
				r = rold;
			}
		}
		return r;
	}
	void disconnect()
	{
		_old = NULL;
	}
public:
	size_t _log_size;
	TEntry * _data;
	CircularArrayImplementation<T> * _old;
};



template< typename T >
CheapSentinel<T> CircularArrayImplementation<T>::uninit_sentinel;

/**
 * @class CircularArray 
 * @brief a lock-free circular array which allows growth.  Setting is
 * done via CAS operations only (to or from NULL).  Access is done via
 * the index type which is interpreted modulo SZ=2^_log_size 
 * of the implementation.  So accessing at i and at i+SZ are the same entry.
 */

template< typename T >
class CircularArray {
public:
	typedef CircularArrayImplementation<T> Implementation;

	typedef TStructEntry<T> TEntry;
	typedef unsigned long index_t;

	CircularArray()
		: _impl(new Implementation())
	{}
	/**
	 * @brief get the i^th entry
	 * @param i is the index accessed
	 * @param update underlying new array if possible
	 * @return pointer to the i^th entry
	 */
	inline TEntry * get(index_t i, bool update=true) const
	{
		TEntry * r = NULL;
		Implementation * impl = NULL;
		while(impl != _impl) {
			impl = _impl;
			r = impl->get(i,update);
		}
		return r;
	}
	inline bool cas_from_null(index_t i, T * n) 
	{
		index_t i_mod;
		bool res = 0;
		Implementation * impl = NULL;
		static T * const u_sentin = Implementation::uninit_sentinel();
		impl = _impl;
		while(impl && impl->_data[i%impl->size()].ptr == u_sentin) {
			impl = impl->_old;
		}
		assert(impl);
		i_mod = i % impl->size();
		(impl->_data)[i_mod].at = i;
		res = __sync_bool_compare_and_swap(
			& ((impl->_data)[i_mod].ptr)
			, NULL
			, n);
		return res;
	}
	inline bool cas_to_null(index_t i, T * o) 
	{
		index_t i_mod;
		bool res = 0;
		Implementation * impl = NULL;
		static T * const u_sentin = Implementation::uninit_sentinel();
		impl = _impl;
		while(impl && impl->_data[i%impl->size()].ptr == u_sentin) {
			impl = impl->_old;
		}
		assert(impl);
		i_mod = i % impl->size();
		impl->_data[i_mod].at = -1;
		res = __sync_bool_compare_and_swap(
			& (impl->_data[i_mod].ptr)
			, o
			, NULL);
		return res;
	}
	inline unsigned long impl_size()
	{ return _impl->size(); }
	/**
	 * @brief double the size of the underlying array
	 * @return true always
	 */
	inline bool grow() 
	{
		Implementation * impl = _impl;
		unsigned long initial_sz = impl->size();
		Implementation * n_impl = new Implementation(impl);
		if(!__sync_bool_compare_and_swap( &_impl, impl, n_impl)) {
			n_impl->disconnect();
			delete n_impl;
			assert(_impl != impl && _impl->size() > initial_sz);
		}
		return true;
	}
private:
	CircularArray(const CircularArray &); // not implemented
public:
	Implementation * _impl;
};

/**
 * @class LFArrayQueue
 * @brief a lock-free queue based on the circular array.  
 * Augments the underlying array with indexes for IN and OUT positions.
 * T is the type of the items managed by the queue.
 */

template< typename T >
class LFArrayQueue : public CircularArray<T> {
public:
	typedef TStructEntry<T> TEntry;
	typedef unsigned long index_t;

	LFArrayQueue()
		: _in(0)
		, _out(0)
	{}
	/**
	 * @brief pop a value if available (NULL if none) from queue
	 * @return a value on success or NULL otherwise
	 */
	inline T * pop() // return NULL when non available
	{
		long retries = 0;
		T * res = NULL;
		TEntry * r = NULL;
		index_t out = 0;
		do {
			out = _out;
			if(out >= _in) {
				res = NULL;
				break; // empty
			}
			r = CircularArray<T>::get(out,false);
				// safe to start updating once past from
			res = r->ptr;
			++retries;
		} while((res != NULL && r->at >= 0
			? !__sync_bool_compare_and_swap(
				  &_out
				, out
				, out+1)
			: 1 /* go around again */));
		if(res) {
			bool cas_res = CircularArray<T>::cas_to_null(out,res);
			assert(cas_res && "must write NULL on pop");
		}
		return res;
	}
	/**
	 * @brief push a non-NULL value (will assert on NULL) into queue
	 * @param o object to push
	 */
	inline void push(T *o) // only allow non-NULL o's to be pushed
	{
		long retries = 0;
		assert(o);
		index_t in = 0;
		while(1) {
			in = _in;
			if((in-_out)+1 >= CircularArray<T>::impl_size()) {
				CircularArray<T>::grow(); // grow it bigger
			}
			TEntry * t = CircularArray<T>::get(in);
			if(!t->ptr
				&& __sync_bool_compare_and_swap(
					  &_in
					, in
					, in+1)) {
				bool cas_res = CircularArray<T>::cas_from_null(in,o);
				assert(cas_res && "must write on push");
				break;
			}
			++retries;
		}
	}
	inline size_t size() const
	{
		return (_in - _out);
	}
private:
	/** idealized invariant: _out <= _in **/
	volatile index_t _in;  // index for pushes
	volatile index_t _out; // index for pops
};

} // namespace growable
} // namespace lockfree
} // namespace oflux

#endif //OFLUX_CIRCULAR_GROWABLE_ARRAY
