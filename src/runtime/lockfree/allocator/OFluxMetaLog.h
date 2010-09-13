#ifndef OFLUX_META_LOG_H
#define OFLUX_META_LOG_H

/**
 * @file OFluxMetaLog.h
 * @author Mark Pichora
 *   Meta-programming computation of logarithms
 */

// find the highest bit of an unsigned value:
// e.g:
// 1 -> 0
// 2 -> 1
// 3 -> 1
// 4 -> 2
// 5 -> 2
// 6 -> 2
// ...

namespace oflux {
namespace lockfree {
namespace allocator {

template<size_t v>
struct Log {
	enum { result = 1 + Log<(v >> 1)>::result };
};

// base cases:

template<>
struct Log<0> {
	Log(const char * msg = "cannot log 0");
};

template<>
struct Log<1> {
	enum { result = 0 };
};

template< size_t divisor
	, bool is_pow_two=((1<<Log<divisor>::result)==divisor)>
struct DivideBy {
	static inline size_t into(size_t onvalue) {
		return onvalue / divisor;
	}
};

template< size_t divisor >
struct DivideBy<divisor,true> {
	static inline size_t into(size_t onvalue) {
		return onvalue >> Log<divisor>::result;
	}
};

} // namespace allocator
} // namespace lockfree
} // namespace oflux


#endif // OFLUX_META_LOG_H
