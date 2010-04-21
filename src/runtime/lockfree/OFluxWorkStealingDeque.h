#ifndef OFLUX_WS_DEQUE
#define OFLUX_WS_DEQUE
//
// based on the paper:
// David Chase and Yososi Lev
// "Dynamic Circular Work-Stealing Deques" (SPAA'05)
//

#include <cstdio>

namespace oflux {
namespace lockfree {

template<typename T>
class CircularArray {
public:
	typedef T * TPtr;
	enum { default_log_size = 3 };
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
public: 
	// sentinels: 
	static T empty;
	static T abort;

	CircularWorkStealingDeque()
		: _bottom(0)
		, _top(0)
		, _active_array(new CircularArray<T>())
	{}

	~CircularWorkStealingDeque()
	{ delete _active_array; }

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
			CircularArray<T> * old_array = _active_array;
			_active_array = a;
			// old_array is not deleted
		}
		a->put(b,e);
		_bottom = b+1;
	}

	inline T * steal()
	{
		long t = _top;
		long b = _bottom;
		CircularArray<T> * a = _active_array;
		long size = b - t;
		if(size <= 0) {
			return &empty;
		}
		T * e = a->get(t);
		if(!cas_top(t,t+1)) {
			return &abort;
		}
		return e;
	}

	inline T * popBottom()
	{
		long b = _bottom;
		CircularArray<T> * a = _active_array;
		b = b-1;
		_bottom = b;
		long t = _top;
		long size = b - t;
		if(size < 0) {
			_bottom = t;
			return &empty;
		}
		T * e = a->get(b);
		if(size>0) {
			return e;
		}
		if(!cas_top(t,t+1)) {
			e = &empty;
		}
		_bottom = t+1;
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

private:
	long _bottom;
	long _top;
	CircularArray<T> * _active_array;
	char _align_dontcare[256 - 2*sizeof(long) - sizeof(CircularArray<T> *)];
};


template<typename T>
T CircularWorkStealingDeque<T>::empty;

template<typename T>
T CircularWorkStealingDeque<T>::abort;

} // namespace lockfree
} // namespace oflux

#endif // OFLUX_WS_DEQUE
