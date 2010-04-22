#ifndef OFLUX_DISTRIBUTED_COUNTER_H
#define OFLUX_DISTRIBUTED_COUNTER_H

#include <cstdlib>

#define DEFAULT_MEMPOOL_MAX_THREADS  10

namespace oflux {

class ThreadNumber {
public:
        static size_t num_threads;
        size_t index;
};

extern __thread ThreadNumber _tn;

extern void init_ThreadNumber(ThreadNumber & tn);

template< typename CT
	, size_t num_threads=DEFAULT_MEMPOOL_MAX_THREADS>
class Counter {
public:
	Counter()
	{
		for(size_t i = 0; i < num_threads; ++i) {
			_v[i] = 0;
		}
	}
	CT operator+=(CT by) { // add by
		return _v[_tn.index] += by;
	}
	CT operator-=(CT by) { // subtract by
		return _v[_tn.index] -= by;
	}
	CT operator++(int) { // pre inc
		return ++_v[_tn.index];
	}
	CT operator++() { // post inc
		return _v[_tn.index]++;
	}
	CT operator--(int) { // pre dec
		return --_v[_tn.index];
	}
	CT operator--() { // post dec
		return _v[_tn.index]--;
	}
	volatile CT & operator[](size_t i) {
		return _v[i];
	}
	CT value() const
	{
		CT res = 0;
		for(size_t i = 0; i < num_threads; ++i) {
			res += _v[i];
		}
		return res;
	}
private:
	volatile CT _v[num_threads];
};

} // namespace oflux

#endif
