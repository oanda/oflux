#ifndef OFLUX_LOCKFREE_ATOMIC
#define OFLUX_LOCKFREE_ATOMIC
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
#include "atomic/OFluxAtomic.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "event/OFluxEventBase.h"
#include "OFluxAllocator.h"
#include "lockfree/allocator/OFluxSMR.h"
#include "OFluxLogging.h"
#include "OFluxRollingLog.h"
#include "flow/OFluxFlowNode.h"
#include <inttypes.h>
#include <strings.h>

/**
 * @file OFluxLFAtomic.h
 * @author Mark Pichora
 * Lock-free version of the exclusive atomic and some common things.
 */

namespace oflux {
namespace lockfree {
namespace atomic {

typedef ::oflux::atomic::AtomicFree AtomicFree;

template<const size_t v, typename T>
inline bool is_val(const T * p)
{
	return reinterpret_cast<size_t>(p) == v;
}

template<const size_t v, typename T>
inline void set_val(T * & p)
{
	reinterpret_cast<size_t &>(p) = v;
}

#define is_one    is_val<0x0001>
#define is_three  is_val<0x0003>
#define set_one   set_val<0x0001>
#define set_three set_val<0x0003>


template<typename T>
inline T * unmk(T * t)
{
	size_t u = reinterpret_cast<size_t>(t);
	return reinterpret_cast<T *>(u & ~0x0001);
}

template<typename T>
inline T * mk(T * p)
{
	size_t u = reinterpret_cast<size_t>(p);
	return reinterpret_cast<T *>(u | 0x0001);
}

template<typename T>
inline bool mkd(T * p)
{
        return reinterpret_cast<size_t>(p) & 0x0001;
}

#define resource_loc_asgn(B,V) \
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT  "] on line %d assigning %p to %p->%p\n" \
		, oflux_self() \
		, __LINE__ \
		, V \
		, B \
		, B->resource_loc)


inline void ev_swap(EventBase * & ev1, EventBase * & ev2)
{
	EventBase * ev = ev1;
	ev1 = ev2;
	ev2 = ev;
}

struct EventBaseHolder {
	enum WType 
		{ None = oflux::atomic::AtomicCommon::None
		, Read = oflux::atomic::AtomicReadWrite::Read
		, Write = oflux::atomic::AtomicReadWrite::Write
		, Exclusive = oflux::atomic::AtomicExclusive::Exclusive 
		, Upgradeable = oflux::atomic::AtomicReadWrite::Upgradeable
		, Pool = oflux::atomic::AtomicPooled::Pool
		, Free = oflux::atomic::AtomicFree::Free
		};

	EventBaseHolder(  EventBasePtr & a_ev
			, int a_type = None)
		: ev(get_EventBasePtr(a_ev))
		, next(NULL)
		, type(a_type)
		, resource_loc(NULL)
		, resource(NULL)
	{
		resource_loc_asgn(this,NULL);
	}
	EventBaseHolder(  EventBasePtr & a_ev
			, EventBaseHolder ** a_resource_loc
			, int a_type = None)
		: ev(recover_EventBasePtr(a_ev))
		, next(NULL)
		, type(a_type)
		, resource_loc(a_resource_loc)
		, resource(NULL)
	{
		assert(ev);
		resource_loc_asgn(this,a_resource_loc);
	}

	EventBaseHolder(  char c
			, EventBaseHolder ** a_resource_loc
			, int a_type = None)
		: ev(NULL)
		, next(NULL)
		, type(a_type)
		, resource_loc(a_resource_loc)
		, resource(NULL)
	{
		resource_loc_asgn(this,a_resource_loc);
	}

	EventBaseHolder( void * a_resource )
		: ev(NULL)
		, next(NULL)
		, type(None)
		, resource_loc(NULL)
		, resource(a_resource)
	{
		resource_loc_asgn(this,NULL);
	}
	EventBaseHolder()
		: ev(NULL)
		, next(NULL)
		, type(None)
		, resource_loc(NULL)
		, resource(NULL)
	{
		resource_loc_asgn(this,NULL);
	}

	inline void busyWaitOnEv()
	{
		size_t retries = 0;
		size_t warning_level = 1;
		while(ev == NULL) {
			if(retries > warning_level) {
				oflux_log_error("EventBaseHolder::busyWaitOnEv retries at %d\n", retries);
				warning_level = std::max(warning_level, warning_level << 1);
			}
			sched_yield();
			store_load_barrier();
			++retries;
		}
	}

	EventBase * ev;
	EventBaseHolder * next;
	int type;
	EventBaseHolder ** resource_loc;
	void * resource;
};

class WaiterList;

typedef ::oflux::lockfree::smr::DeferFree DeferFree;
//typedef ::oflux::DefaultDeferFree DeferFree;

class AtomicCommon : public oflux::atomic::Atomic {
public:
	static Allocator<EventBaseHolder,DeferFree> allocator;

	AtomicCommon(void * d)
		: _data_ptr(d)
	{}
	virtual ~AtomicCommon();
	virtual void ** data() { return &_data_ptr; }
	virtual void relinquish(bool) {} // nothing to do
protected:
	static void _log_snapshot_waiters(const WaiterList *);
protected:
	void * _data_ptr;
};

class WaiterList {
public:
	typedef EventBaseHolder::WType WType;

	WaiterList()
		: _head(AtomicCommon::allocator.get(EventBase::no_event, EventBaseHolder::None))
		, _tail(_head)
	{ set_val<0x0001>(_head->next); }
	inline bool empty() const { return is_val<0x0001>(_head->next); }
	inline size_t count() const // number of waiters
	{
		size_t res = 0;
		EventBaseHolder * h = _head;
		while(h && !is_val<0x0001>(h) && !is_val<0x0003>(h) && h->ev) {
			++res;
			h = h->next;
		}
		return res;
	}
	inline bool has_waiters() const
	{
		EventBaseHolder * h = _head->next;
		return !is_val<0x0001>(h) && (h != NULL);
	}
public:
	EventBaseHolder * _head;
	EventBaseHolder * _tail;
};

/**
 * @class ExclusiveWaiterList
 * @brief Lock-free structure used to control access to the AtomicExclusive
 */

class ExclusiveWaiterList : public WaiterList {
public:
//#define LF_EX_WAITER_INSTRUMENTATION
#ifdef LF_EX_WAITER_INSTRUMENTATION
	struct Observation {
		enum    { Action_none
			, Action_A_o_w
			, Action_Rel 
			, Action_a_o_w
			, Action_rel 
			} action;
		const char * trans;
		int res;
		const EventBaseHolder * e;
		const EventBaseHolder * h;
		const EventBaseHolder * hn;
		const EventBaseHolder * t;
                EventBaseHolder * const * cas_addr;
                const EventBaseHolder * cas_val;
		unsigned retries;
		pthread_t tid;
		EventBase * ev;
		int tail_up;
		long long term_index;
	};

	RollingLog<Observation> log;
#endif // LF_EX_WAITER_INSTRUMENTATION

	~ExclusiveWaiterList();
	bool push(EventBaseHolder * e);
	EventBaseHolder * pop();
	void dump();
};

/**
 * @class AtomicExclusive
 * @brief Lock-free exclusive atomic.  
 *   These objects are used in exclusive guards by the LF runtime to
 *   control access to guard entries.
 */

class AtomicExclusive : public AtomicCommon {
public:
	AtomicExclusive(void * data)
		: AtomicCommon(data)
	{}
	virtual ~AtomicExclusive() {}
	virtual int held() const { return ! _waiters.empty(); }
	virtual size_t waiter_count() { return _waiters.count(); }
	virtual bool has_no_waiters() { return !_waiters.has_waiters(); }
	virtual int wtype() const { return EventBaseHolder::Exclusive; }
	virtual const char * atomic_class() const
	{ return "lockfree::AtomicExclusive"; }
	virtual void release(
		  std::vector<EventBasePtr > & rel_ev
                , EventBasePtr &)
	{
		EventBaseHolder * ebh = _waiters.pop();
		if(ebh) {
			set_three(ebh->next);
			rel_ev.push_back(EventBasePtr(ebh->ev));
			AtomicCommon::allocator.put(ebh);
		}
	}
	virtual bool acquire_or_wait(EventBasePtr & ev, int wtype)
	{
		EventBaseHolder * ebh = allocator.get(ev,wtype);
		bool acqed = _waiters.push(ebh);
		if(acqed) {
			set_three(ebh->next);
			AtomicCommon::allocator.put(ebh); // not in use - return it to pool
		} else {
			checked_recover_EventBasePtr(ev);
		}
		return acqed;
	}
	virtual void log_snapshot_waiters() const
	{ _log_snapshot_waiters(&_waiters); }

	void _dump()
	{ _waiters.dump(); }
private:
	ExclusiveWaiterList _waiters;
};

} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC
