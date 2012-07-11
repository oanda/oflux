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
#include "OFluxThreadNumber.h"

namespace oflux {
namespace lockfree {

size_t ThreadNumber::num_threads = 0;
__thread ThreadNumber _tn = {0};

void
ThreadNumber::init(int i)
{
	// could have assigned the index implicitly here,
	// but I prefer to guarantee that the ith item is indexed on
	// (i-1) index
	_tn.index = i;
	__sync_fetch_and_add(&ThreadNumber::num_threads,1);
}

void
ThreadNumber::init()
{
	// This version is less used
	_tn.index = __sync_fetch_and_add(&ThreadNumber::num_threads,1);
}


} // namespace lockfree
} // namespace oflux
