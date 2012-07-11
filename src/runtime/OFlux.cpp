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
#include "OFlux.h"
#include <sys/types.h>
#include <time.h>

namespace oflux {

/**
 * @brief instance of the empty structure
 */
E theE;

// Define DELTA to be the inaccuracy you can tolerate
#define DELTA 10000000   /* 10 millisecond */

/*
#ifdef LINUX
// for linux, this may not be worthwhile - since the time() call may be faster
// than the gethrtime() calls
typedef long long hrtime_t;

hrtime_t  gethrtime(void)
{
	struct timespec sp;
	int ret;
	long long v;
#ifdef CLOCK_MONOTONIC_HR
	ret=clock_gettime(CLOCK_MONOTONIC_HR, &sp);
#else
	ret=clock_gettime(CLOCK_MONOTONIC, &sp);
#endif
	if(ret!=0) return 0;
	v=1000000000LL; // seconds->nanonseconds
	v*=sp.tv_sec;
	v+=sp.tv_nsec;
	return v;
}
#endif
*/

#if defined LINUX || defined Darwin
time_t fast_time(time_t *tloc)
{
	return time(tloc);
}
#else
time_t fast_time(time_t *tloc)
{
	static time_t global = 0;
	static hrtime_t old = 0;

	hrtime_t newtime = gethrtime();

	if(newtime - old > DELTA ){
		global = time(tloc);
		old = newtime;
	}

	return global;
}
#endif

};
