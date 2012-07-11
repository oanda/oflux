#ifndef OFLUX_SHARED_POINTER
#define OFLUX_SHARED_POINTER
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

#include <cassert>
#include <cstdlib>
#include <iostream>

//
// The boost one is just not the ticket
//

namespace oflux {

template<typename T>
class shared_ptr {
public:
	explicit shared_ptr()
		: _px(NULL)
		, _pn(NULL)
	{}
	explicit shared_ptr(T *p)
		: _px(p)
		, _pn(p ? new int(1) : NULL)
	{
	}
	shared_ptr(const shared_ptr<T> & sp)
		: _px(sp._px)
		, _pn(sp._pn)
	{
		if(_pn) {
			__sync_fetch_and_add(_pn,1);
		}
	}
	~shared_ptr()
	{
		unlink();
	}
	operator bool () const
	{
		return _px != 0;
	}
	inline shared_ptr<T> & operator=(const shared_ptr<T> &sp)
	{
		unlink();
		if(sp._pn) {
			_px = sp._px;
			_pn = sp._pn;
			if(_pn) {
				__sync_fetch_and_add(_pn,1);
			}
		}
		return *this;
	}
	T * operator->() const
	{
		assert(_px);
		return _px;
	}
	inline shared_ptr<T> & operator=(T * p)
	{
		unlink();
		if(p) {
			_pn = new int(1);
			_px = p;
		}
		return *this;
	}
	inline void reset()
	{
		unlink();
	}
	inline void reset(T * p)
	{
		*this = p;
	}
	inline bool operator==(const shared_ptr<T> & sp) const
	{
		return _px == sp._px
			&& _pn == sp._pn;
	}
	inline void swap(shared_ptr<T> & sp)
	{
		int * pn = sp._pn;
		T * px = sp._px;
		sp._pn = _pn;
		_pn = pn;
		sp._px = _px;
		_px = px;
	}
	inline T * get() const 
	{ return _px; }
	inline bool unique() const
	{ return use_count() == 1; }
	inline int use_count() const
	{
		return _pn ? *_pn : 0;
	}
	inline T * recover()
	{
		T * tptr = NULL;
		if(unique()) {
			tptr = _px;
			unlink(true);
		}
		return tptr;
	}
protected:
	inline void unlink(bool no_delete = false)
	{
		if(_pn) {
			int r = __sync_add_and_fetch(_pn,-1);
			if(r == 0) {
				if(!no_delete) {
					delete _px;
				}
				delete _pn;
			}	
			_pn = NULL;
		}
		_px = NULL;
	}
private:
	T *   _px;
	int * _pn;
};

template<typename T>
std::ostream& operator<<(std::ostream& o, const shared_ptr<T> & sp)
{
	o << sp.get();
	return o;
}

} // namespace oflux

#endif // OFLUX_SHARED_POINTER
