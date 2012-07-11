#ifndef OFLUX_ROLLING_LOG
#define OFLUX_ROLLING_LOG
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

#include <string.h>
#include <strings.h>

// This class is used as portable instrumentation when needed

namespace oflux {

template<typename D, const size_t sz=4*4096> // D is POD
class RollingLog {
public:
	enum { log_size = sz, d_size = sizeof(D) };
	RollingLog()
		: index(1LL)
	{
		::bzero(&(log[0]),sizeof(log));
	}
	inline D & submit()
	{
		long long i = __sync_fetch_and_add(&index,1);
		log[i%log_size].index = i;
		return log[i%log_size].d;
	}
	inline long long at() { return index; }
protected:
	long long index;
	struct S { long long index; D d; } log[log_size];
};

} // namespace oflux

#endif // OFLUX_ROLLING_LOG
