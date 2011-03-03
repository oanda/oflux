#ifndef OFLUX_SENTINEL_H
#define OFLUX_SENTINEL_H

namespace oflux {
namespace lockfree {

// does not assume that T has no pure virtuals.  T can be almost anything.

template< typename T >
class CheapSentinel { // meant to be a static instance
public:
	CheapSentinel() : _dontcare(0) {}
	T * operator()() 
	{ return static_cast<T *>(reinterpret_cast<void *>(&_dontcare)); }
private:
	int _dontcare;
};

} // namespace lockfree
} // namespace oflux

#endif // OFLUX_SENTINEL_H
