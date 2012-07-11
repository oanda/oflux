#ifndef OFLUX_META_LOG_H
#define OFLUX_META_LOG_H
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
