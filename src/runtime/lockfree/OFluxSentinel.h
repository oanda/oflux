#ifndef OFLUX_SENTINEL_H
#define OFLUX_SENTINEL_H
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
