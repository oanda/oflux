#ifndef OFLUX_CIRCULAR_GROWABLE_ARRAY2
#define OFLUX_CIRCULAR_GROWABLE_ARRAY2

#include <cassert>
#include <cstdio>


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
	T * ptr;
	long at;
};

template< typename T >
class CircularArrayImplementation {
public:
	enum { default_log_size = 4 };

	static CheapSentinel<T> uninit_sentinel;

	typedef TStructEntry<T> TEntry;
	

	typedef T * TPtr;


	CircularArrayImplementation()
		: _log_size(default_log_size)
		, _old(NULL)
	{
		_data = new TEntry[1<< _log_size];
		_data[0].ptr = NULL;
		_data[0].at = -1;
		for(long i = 1; i < size(); ++i) {
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
		for(long i = 1; i < size(); ++i) {
			_data[i].ptr = _data[0].ptr;
			_data[i].at = -1;
		}
	}
	~CircularArrayImplementation()
        {
                delete [] _data;
                delete _old;
        }
        inline long size() const
        { return 1 << _log_size; }
        inline TEntry * get(long i, bool update = true) const
        {
		long i_mod = i % size();
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
					/*printf("get() ____ block %ld "
						"rold/%ld (%p,%ld) "
						"r/%ld (%p,%ld) "
						"cas %d\n"
						, i
						, _old->size(), rold->ptr, rold->at
						, size(), r->ptr, r->at
						, cas_res);*/
				} else { // was NULL
					//_data[i_mod].at = -1; should already be
					bool cas_res =
						__sync_bool_compare_and_swap(
							& (_data[i_mod].ptr)
							, u_sentin
							, NULL);
					/*printf("get() null block %ld "
						"rold/%ld (%p,%ld) "
						"r/%ld (%p,%ld) "
						"cas %d\n"
						, i
						, _old->size(), rold->ptr, rold->at
						, size(), r->ptr, r->at
						, cas_res);*/
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


template< typename T >
class CircularArray {
public:
	typedef CircularArrayImplementation<T> Implementation;

	typedef TStructEntry<T> TEntry;

	CircularArray()
		: _impl(new Implementation())
	{}
	inline TEntry * get(long i, bool update=true) const
	{
		TEntry * r = NULL;
		Implementation * impl = NULL;
		while(impl != _impl) {
			impl = _impl;
			r = impl->get(i,update);
		}
		return r;
	}
	inline bool cas_from_null(long i, T * n) 
	{
		long i_mod;
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
	inline bool cas_to_null(long i, T * o) 
	{
		long i_mod;
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
	inline long impl_size()
	{ return _impl->size(); }
	inline bool grow() 
		// make it 2x bigger (ret true if you did it)
		// given i is the starting point for the copy
		//  which indicates where to copy things to
	{
		Implementation * impl = _impl;
		long initial_sz = impl->size();
		//printf(" grown from size %ld \n", initial_sz);
		Implementation * n_impl = new Implementation(impl);
		if(!__sync_bool_compare_and_swap( &_impl, impl, n_impl)) {
			n_impl->disconnect();
			delete n_impl;
			//printf(" deleted\n");
			assert(_impl != impl && _impl->size() > initial_sz);
		}
		return true;
	}
private:
	CircularArray(const CircularArray &); // not implemented
public:
	Implementation * _impl;
};

template< typename T >
class LFArrayQueue : public CircularArray<T> {
public:
	typedef TStructEntry<T> TEntry;

	LFArrayQueue()
		: _in(0)
		, _out(0)
	{}
	inline T * pop() // return NULL when non available
	{
		long retries = 0;
		T * res = NULL;
		TEntry * r = NULL;
		long out = 0;
		/*
		const size_t ie_sz = 5;
		struct IE {
			long out;
			size_t ind;
			CircularArrayImplementation<T> * impl;
			T * ptr;
			long at;
		} ies[ie_sz];
		ies[0].impl = this->_impl;
		for(size_t k = 0; (k < ie_sz-1) && ies[k].impl ; ++k) {
			ies[k].out = _out;
			ies[k].ind = ies[k].out % ies[k].impl->size();
			ies[k].ptr = ies[k].impl->_data[ies[k].ind ].ptr;
			ies[k].at = ies[k].impl->_data[ies[k].ind ].at;
			ies[k+1].impl = ies[k].impl->_old;
		}*/
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
	inline void push(T *o) // only allow non-NULL o's to be pushed
	{
		long retries = 0;
		assert(o);
		long in = 0;
		/*
		const size_t ie_sz = 5;
		struct IE {
			long in;
			size_t ind;
			CircularArrayImplementation<T> * impl;
			T * ptr;
			long at;
		} ies[ie_sz];
		ies[0].impl = this->_impl;
		for(size_t k = 0; (k < ie_sz-1) && ies[k].impl ; ++k) {
			ies[k].in = _in;
			ies[k].ind = ies[k].in % ies[k].impl->size();
			ies[k].ptr = ies[k].impl->_data[ies[k].ind ].ptr;
			ies[k].at = ies[k].impl->_data[ies[k].ind ].at;
			ies[k+1].impl = ies[k].impl->_old;
		}
		*/
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
	/** invariant: _out <= _in **/
	volatile long _in; // index for pushes
	volatile long _out; // index for pops
};

} // namespace growable
} // namespace lockfree
} // namespace oflux

#endif //OFLUX_CIRCULAR_GROWABLE_ARRAY
