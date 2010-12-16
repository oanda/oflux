#ifndef OFLUX_SHARED_POINTER
#define OFLUX_SHARED_POINTER

#include <cassert>
#include <cstdlib>

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
		, _pn(new int(1))
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
	inline bool operator==(const shared_ptr<T> & sp)
	{
		return _px == sp._px
			&& _pn == sp.pn;
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


} // namespace oflux

#endif // OFLUX_SHARED_POINTER
