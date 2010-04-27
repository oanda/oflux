#ifndef OFLUX_LOCKFREE_ATOMIC
#define OFLUX_LOCKFREE_ATOMIC

#include "atomic/OFluxAtomic.h"
#include "event/OFluxEventBase.h"
#include "boost/shared_ptr.hpp"
#include "OFluxAllocator.h"

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

template<typename T>
inline T * unmk(T * t)
{
	return (is_val<0x0001>(t) ? NULL : t);
}

struct EventBaseHolder {
	enum WType { None = 0, Read = 1, Write = 2, Exclusive = 3 };

	EventBaseHolder(
			  EventBasePtr & a_ev
			, int a_type = None)
		: ev(a_ev)
		, next(NULL)
		, type(a_type)
	{}

	EventBasePtr ev;
	EventBaseHolder * next;
	int type;
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
			allocator.put(ebh);
		}
	}
	virtual bool acquire_or_wait(EventBasePtr & ev, int wtype)
	{
		EventBaseHolder * ebh = allocator.get(ev,wtype);
		bool acqed = _waiters.push(ebh);
		if(acqed) {
			allocator.put(ebh); // not in use - return it to pool
		}
		return acqed;
	}
	virtual void log_snapshot_waiters() const
	{ _log_snapshot_waiters(&_waiters); }
private:
	ExclusiveWaiterList _waiters;
};

class ReadWriteWaiterList : public WaiterList {
public:
	ReadWriteWaiterList() : rcount(0) {}

	bool push(EventBaseHolder * e, int type);
	void pop(EventBaseHolder * & el, EventBasePtr & by_e);
	void dump();
	
	int rcount;
};

class AtomicReadWrite : public AtomicCommon {
public:
	AtomicReadWrite(void * data)
		: AtomicCommon(data)
	{}
	virtual ~AtomicReadWrite() {}
	virtual int held() const { return ! _waiters.empty(); }
	virtual size_t waiter_count() { return _waiters.count(); }
	virtual int wtype() const { return _wtype; }
	virtual const char * atomic_class() const
	{ return "lockfree::AtomicReadWrite"; }
	virtual bool acquire_or_wait(EventBasePtr & ev, int wtype)
	{
		EventBaseHolder * ebh = allocator.get(ev,wtype);
		bool acqed = _waiters.push(ebh,wtype);
		if(acqed) {
			_wtype = wtype;
			if(wtype == EventBaseHolder::Read) {
				allocator.put(ebh); 
			}
		}
		return acqed;
	}
	virtual void release(
		  std::vector<EventBasePtr > & rel_ev
                , EventBasePtr & by_e)
	{
		EventBaseHolder * el = NULL;
		_waiters.pop(el,by_e);
		EventBaseHolder * e = el;
		EventBaseHolder * n_e = NULL;
		while(e) {
			n_e = e->next;
			_wtype = e->type;
			rel_ev.push_back(e->ev);
			allocator.put(e);
			e = n_e;
		}
	}
	virtual void log_snapshot_waiters() const;
private:
	ReadWriteWaiterList _waiters;
	int _wtype;
};

} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC
