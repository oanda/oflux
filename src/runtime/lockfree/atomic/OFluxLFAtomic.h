#ifndef OFLUX_LOCKFREE_ATOMIC
#define OFLUX_LOCKFREE_ATOMIC

#include <inttypes.h>
#include "atomic/OFluxAtomic.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "event/OFluxEventBase.h"
#include "boost/shared_ptr.hpp"
#include "OFluxAllocator.h"

#include "OFlux.h"
#include "OFluxLogging.h"
#include "flow/OFluxFlowNode.h"

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
	oflux_log_trace2("[%d] on line %d assigning %p to %p->%p\n" \
		, oflux_self() \
		, __LINE__ \
		, V \
		, B \
		, B->resource_loc)

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
		: ev(a_ev)
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
		: ev(a_ev)
		, next(NULL)
		, type(a_type)
		, resource_loc(a_resource_loc)
		, resource(NULL)
	{
		resource_loc_asgn(this,a_resource_loc);
	}

	EventBaseHolder(  char c
			, EventBaseHolder ** a_resource_loc
			, int a_type = None)
		: ev()
		, next(NULL)
		, type(a_type)
		, resource_loc(a_resource_loc)
		, resource(NULL)
	{
		resource_loc_asgn(this,a_resource_loc);
	}

	EventBaseHolder( void * a_resource )
		: ev()
		, next(NULL)
		, type(None)
		, resource_loc(NULL)
		, resource(a_resource)
	{
		resource_loc_asgn(this,NULL);
	}
	EventBaseHolder()
		: ev()
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
		while(ev.get() == NULL) {
			if(retries > warning_level) {
				oflux_log_error("EventBaseHolder::busyWaitOnEv retries at %d\n", retries);
				warning_level = std::max(warning_level, warning_level << 1);
			}
			sched_yield();
			store_load_barrier();
			++retries;
		}
	}

	EventBasePtr ev;
	EventBaseHolder * next;
	int type;
	EventBaseHolder ** resource_loc;
	void * resource;
};

class WaiterList;

class AtomicCommon : public oflux::atomic::Atomic {
public:
	static Allocator<EventBaseHolder> allocator;

	AtomicCommon(void * d)
		: _data_ptr(d)
	{}
	virtual ~AtomicCommon();
	virtual void ** data() { return &_data_ptr; }
	virtual void relinquish() {} // nothing to do
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
		while(h && !is_val<0x0001>(h) && !is_val<0x0003>(h) && h->ev.get()) {
			++res;
			h = h->next;
		}
		return res;
	}
public:
	EventBaseHolder * _head;
	EventBaseHolder * _tail;
};

class ExclusiveWaiterList : public WaiterList {
public:
	~ExclusiveWaiterList();
	bool push(EventBaseHolder * e);
	EventBaseHolder * pop();
	void dump();
};

class AtomicExclusive : public AtomicCommon {
public:
	AtomicExclusive(void * data)
		: AtomicCommon(data)
	{}
	virtual ~AtomicExclusive() {}
	virtual int held() const { return ! _waiters.empty(); }
	virtual size_t waiter_count() { return _waiters.count(); }
	virtual int wtype() const { return EventBaseHolder::Exclusive; }
	virtual const char * atomic_class() const
	{ return "lockfree::AtomicExclusive"; }
	virtual void release(
		  std::vector<EventBasePtr > & rel_ev
                , EventBasePtr &)
	{
		EventBaseHolder * ebh = _waiters.pop();
		if(ebh) {
			rel_ev.push_back(ebh->ev);
			AtomicCommon::allocator.put(ebh);
		}
	}
	virtual bool acquire_or_wait(EventBasePtr & ev, int wtype)
	{
		EventBaseHolder * ebh = allocator.get(ev,wtype);
		bool acqed = _waiters.push(ebh);
		if(acqed) {
			AtomicCommon::allocator.put(ebh); // not in use - return it to pool
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
