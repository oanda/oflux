#ifndef _OFLUX_SCATTER_GATHER_H
#define _OFLUX_SCATTER_GATHER_H
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

#include "OFluxLogging.h"

/**
 * @file OFluxScatterGather.h
 * @author Mark Pichora
 *   These classes are used to implement scatter/gather patterns in
 *   OFlux programs.
 *
 *   The OFlux nodes involved will take on one of the following roles:
 *   Initial-Splay-And-Creation: will new a Gather-subclass G
 *      and splay outputs to consuming nodes with Scatter<G>::clone() calls
 *   Passing-Through: will copy the received Scatter<G> object from input
 *      to outputs (operator=).  Of course on the way through, this node may
 *      mutate the G going by.
 *   Gathering-Point: using a conditional abstract node to check that G.count()
 *      is 1, the gather point node is entered just once to process the 
 *      completed (scattered) computation.
 */
namespace oflux {

/**
 * @class Gatherable
 * @brief a base class for objects that will gather processed data in one place
 */
class Gatherable {
public:
	Gatherable() : _ref_count(0) {}

	/**
	 * @brief increment a reference count (MT safe)
	 * @return post increment value
	 */
	size_t 
	incr() 
	{ return __sync_fetch_and_add(&_ref_count,1); }

	/**
	 * @brief decrement a reference count (MT safe)
	 * @return post decrement value
	 */
	size_t 
	decr() 
	{ 
		if(_ref_count<=0) {
			oflux_log_error("oflux::Gatherable::decr() passing 0\n");
		}
		return __sync_fetch_and_add(&_ref_count,-1); 
	}

	/**
	 * @brief get the current reference count
	 * @return reference count
	 */
	size_t 
	count() const 
	{ return _ref_count; }
private:
	volatile size_t _ref_count;
};

/**
 * @class Scatter
 * @brief given a typename P subclass of Gatherable, this convenience class
 *  is used to pass and share the unique object need in a flow to gather 
 *  a computation
 *
 *  Typically there will be several Scatter<P> instances which point to a P.
 */
template<typename P /* implements Gatherable */ >
class Scatter {
public:
	Scatter()
		: _p(NULL)
	{ }

	Scatter(P * p)
		: _p(p)
	{ p->incr(); }

	~Scatter()
	{ disown(); }

	/**
	 * @brief obtain the underlying gatherable object
	 */
	P * 
	getGatherable() { return _p; }

	/**
	 * @brief obtain the underlying gatherable object
	 */
	const P * 
	getGatherable() const { return _p; }

	/**
	 * @brief copy but pass ownership (no change to ref count)
	 * @param sp reference to take over
	 * @return a reference to the result (C++ standard)
	 */
	Scatter<P> & 
	operator=(const Scatter<P> & sp)
	{ // pass ownership
		disown();
		_p = sp._p;
		sp._p = NULL;
		return *this;
	}

	P *
	operator->() { return _p; }

	const P *
	operator->() const { return _p; }

	/**
	 * @brief make a copy and increase reference count (share P)
	 * @param sp is one of the Scatter<P> references
	 */
	void
	clone(const Scatter<P> & sp)
	{ // incr ref count
		if(_p == sp._p) { return; }
		disown();
		if(sp._p) {
			_p = sp._p;
			_p->incr();
		}
	}
private:
	/**
	 * @brief cause this Scatter<P> to remove itself from the 
	 *    P reference count and delete if necessary (last one)
	 */
	void 
	disown() 
	{
		if(_p) { 
			_p->decr(); 
			if(!_p->count()) {
				delete _p;
			}
			_p = NULL;
		}
	}
private:
	mutable P * _p;
};

} // namespace oflux

#endif
