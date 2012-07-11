#ifndef OFLUX_THREAD_NUMBER
#define OFLUX_THREAD_NUMBER
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
 * @file OFluxThreadNumber.h
 * @author Mark Pichora
 *  Portable thread numbering which allows the program to know the total number
 * of threads and the index of the current thread (in thread local storage).
 */

#include <cstdlib>
#include <pthread.h>

namespace oflux {
namespace lockfree {

class ThreadNumber {
public:
        static size_t num_threads;
        size_t index;

	static void init(int i);
	static void init();
};

extern __thread ThreadNumber _tn; // thread local access to a thread's index


} // namespace lockfree
} // namespace oflux


#endif // OFLUX_THREAD_NUMBER
