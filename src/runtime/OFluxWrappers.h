#ifndef _OFLUX_WRAPPERS
#define _OFLUX_WRAPPERS

/**
 * @file OFluxWrappers.h
 * @author Mark Pichora
 * Just some nice to have wrappers around pthreads objects.
 * RAII is extensive.
 */

#include "OFlux.h"
#include "OFluxThreads.h"
#ifdef OFLUX_RT_DEBUG
# include "OFluxLogging.h"
#endif


namespace oflux {

/**
 * @class AutoLock
 * @brief RAII class for holding a lock
 */
class AutoLock {
public:
	AutoLock(oflux_mutex_t * l)
		: _lck(l)
	{ oflux_mutex_lock(_lck); }
	~AutoLock()
	{ oflux_mutex_unlock(_lck); }
private:
	oflux_mutex_t * _lck;
};

/**
 * @class AutoUnlock
 * @brief RAII converse operation of the lock holder (a releaser)
 */
class AutoUnLock {
public:
	AutoUnLock(oflux_mutex_t* l)
		: _lck(l)
	{ oflux_mutex_unlock(_lck); }

	~AutoUnLock()
	{ oflux_mutex_lock(_lck); }
private:
	oflux_mutex_t * _lck;
};

class IntegerCounter {
public:
	IntegerCounter()
		: _c(0)
		{}
	inline IntegerCounter & operator++() // ++c
		{
			++_c;
			return *this;
		}
	inline IntegerCounter & operator--() // --c
		{
			--_c;
			return *this;
		}
	inline const int & operator()()
		{
			return _c;
		}
	inline bool operator==(const int & a)
		{
			return _c == a;
		}
protected:
	int _c;
};

/**
 * @class ConditionVariable
 * @brief A container for a condition variable, with retained signals()
 * This class will hold an outstanding signal until one of the incoming
 * threads does a conditional wait.
 */
template<class C>
class ConditionVariable {
public:
	ConditionVariable(oflux_mutex_t* l)
		: _lck(l)
		, _allow_skip_cond(false)
	{
		oflux_cond_init(&_cond);
	}
	~ConditionVariable()
	{
		oflux_cond_destroy(&_cond);
	}
	void wait()
	{
#ifdef OFLUX_RT_DEBUG
		oflux_log_debug(" %x : wait() this %x wc %d asc %d\n"
                        , oflux_self()
			, this
			, (const int &)_waiter_count
			, _allow_skip_cond);
#endif
		++_waiter_count;
		if(_allow_skip_cond) {
			_allow_skip_cond = false;
		} else {
			oflux_cond_wait(&_cond, _lck);
		}
		--_waiter_count;
	}
	void signal()
	{
#ifdef OFLUX_RT_DEBUG
		oflux_log_debug(" %x : signal() this %x wc %d asc %d\n"
			, oflux_self()
			, this
			, (const int &)_waiter_count
			, _allow_skip_cond);
#endif
		if(_waiter_count==0) {
			_allow_skip_cond = true;
		}
		oflux_cond_signal(&_cond);
	}
        void turn_off()
        {
                _allow_skip_cond = true;
		oflux_cond_broadcast(&_cond);
        }
	const int & count() const { return (const int &) _waiter_count; }
	inline C & counter_implementation() { return _waiter_count; }
private:
	oflux_mutex_t *   _lck;
	C                 _waiter_count;
	bool              _allow_skip_cond;
	oflux_cond_t      _cond;
};

/**
 * @class SetTrue
 * @brief RAII set a boolean variable to true during lifetime
 */
class SetTrue {
public:
	SetTrue(bool & val)
		: _val(val)
		, _valr(val)
		{ val = true; }
	~SetTrue() { _valr = _val; } // revert to old value
private:
	bool _val;
	bool & _valr;
};

/**
 * @class Increment
 * @brief RAII to increment a value and then decrement it again when done
 * not MT safe
 */
class Increment {
public:
	Increment(int & val, oflux_thread_t whoami)
		: _valp(&val)
		{ val++; }
	~Increment() { if(_valp) (*_valp)--; } // count down one
	void release(oflux_thread_t whoami)
		{
		  if(_valp) {
			(*_valp)--;
		  }
		  _valp = NULL;
		}
private:
	int * _valp;
};


/**
 * @class ThreadLocalDataKey
 * @brief a container for a thread local data key
 */
template<class T>
class ThreadLocalDataKey {
public:
	ThreadLocalDataKey(void (*destructor)(void *))
	{
		oflux_key_create(&_thread_data_key,destructor);
	}
	~ThreadLocalDataKey()
	{
		oflux_key_delete(_thread_data_key);
	}
	T* get()
	{
		return (T*)(oflux_get_specific(_thread_data_key));
	}
	void set(T * t)
	{
		oflux_set_specific(_thread_data_key,t);
	}
private:
	oflux_key_t                _thread_data_key;
};

}; //namespace

#endif // _OFLUX_WRAPPERS
