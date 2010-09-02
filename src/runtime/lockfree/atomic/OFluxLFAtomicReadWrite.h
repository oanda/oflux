#ifndef OFLUX_LOCKFREE_ATOMIC_RW
#define OFLUX_LOCKFREE_ATOMIC_RW

#include "OFluxLFAtomic.h"

namespace oflux {
namespace lockfree {
namespace atomic {

class RWWaiterHead {
public:
	RWWaiterHead()
	{ u.u64 = 0LL; }
	RWWaiterHead(EventBaseHolder * e, int rc, bool m)
	{
		u.s.h = e;
		u.s.rcount_mkd = (rc<<1) | (m ? 0x0001: 0x0000);
	}
	RWWaiterHead(const RWWaiterHead & o)
	{
		u.u64 = o.u.u64;
	}
	RWWaiterHead & operator=(const RWWaiterHead & o)
	{
		u.u64 = o.u.u64;
		return *this;
	}
	inline EventBaseHolder * head() const
	{ return u.s.h; }
	inline bool mkd() const
	{ return u.s.rcount_mkd & 0x0001; }
	inline int rcount() const
	{ return u.s.rcount_mkd >> 1; }
	inline bool compareAndSwap(
		  const RWWaiterHead & old_o
		, const RWWaiterHead & new_o)
	{
		return __sync_bool_compare_and_swap(
			  &(u.u64)
			, old_o.u.u64
			, new_o.u.u64);
	}
	inline bool compareAndSwap(
		  const RWWaiterHead & old_o
		, EventBaseHolder * const e
		, const int rc
		, bool m)
	{
		RWWaiterHead rwh(e,rc,m);
		return compareAndSwap(old_o,rwh);
	}
public:
	union U {
		struct S {
			EventBaseHolder * h;
			int rcount_mkd;
		} s;
		uint64_t u64;
	} u;
};

class ReadWriteWaiterList {
public:
	ReadWriteWaiterList()
		: _head(AtomicCommon::allocator.get(EventBase::no_event,EventBaseHolder::None), 0, false)
		, _tail(_head.head())
	{}
	~ReadWriteWaiterList();
	bool push(EventBaseHolder * e, int type);
	void pop(EventBaseHolder * & el, int by_type);
	void dump();
	size_t count() const;
public:
	RWWaiterHead _head;
	EventBaseHolder * _tail;
};

class AtomicReadWrite : public AtomicCommon {
public:
	AtomicReadWrite(void * data)
		: AtomicCommon(data)
		, _wtype(EventBaseHolder::None)
	{}
	virtual ~AtomicReadWrite() {}
	virtual int held() const 
	{ 
		int rcount = _waiters._head.rcount();
		return rcount < 0 ? 1 : rcount;
	}
	virtual size_t waiter_count() 
	{ return _waiters.count(); }
	virtual int wtype() const { return _wtype; }
	virtual const char * atomic_class() const
	{ return "lockfree::AtomicReadWrite"; }
	virtual bool acquire_or_wait(EventBasePtr & ev, int wtype)
	{
		EventBaseHolder * ebh = allocator.get(ev,wtype);
		// Upgradable could be stuck into the lower-level
		// lf queue, but the added complexity is probably not worth it
		//  we tolerate: that sometimes upgradable will grab Write
		//  access to non-NULL data (which the app will treat as readonly.
		// we guarantee that NULL data is not grabbed for Read.
		int local_wtype =
			( wtype == EventBaseHolder::Upgradeable
			? ( *data()
			    ? EventBaseHolder::Read 
			    : EventBaseHolder::Write)
			: wtype);
		bool acqed = _waiters.push(
			  ebh
			, local_wtype);
		if(acqed) {
			oflux_log_trace2("RW::a_o_w %s %p %p acqed %d %d\n"
				, ev->flow_node()->getName()
				, ev.get()
				, this
				, wtype
				, _waiters._head.rcount());
			_wtype = local_wtype;
			AtomicCommon::allocator.put(ebh); 
		} else {
			oflux_log_trace2("RW::a_o_w %s %p %p waited %d %d\n"
				, ev->flow_node()->getName()
				, ev.get()
				, this
				, wtype
				, _waiters._head.rcount());
		}
		return acqed;
	}
	virtual void release(
		  std::vector<EventBasePtr > & rel_ev
                , EventBasePtr & by_e)
	{
		EventBaseHolder * el = NULL;
		oflux_log_trace2("RW::rel   %s %p %p releasing %d %d\n"
			, by_e->flow_node()->getName()
			, by_e.get()
			, this
			, _wtype
			, _waiters._head.rcount());
		_waiters.pop(el,_wtype);
		//_wtype = EventBaseHolder::None;
		EventBaseHolder * e = el;
		EventBaseHolder * n_e = NULL;
		while(e) {
			n_e = e->next;
			_wtype = e->type;
			oflux_log_trace2("RW::rel   %s %p %p came out %d %d\n"
				, e->ev.get() ? e->ev->flow_node()->getName() : "<null>"
				, e->ev.get()
				, this
				, e->type
				, _waiters._head.rcount());
			//_wtype = e->type;
			if(e->ev.get()) { 
				rel_ev.push_back(e->ev); 
			}
			AtomicCommon::allocator.put(e);
			e = n_e;
		}
	}
	virtual void log_snapshot_waiters() const {} // TODO
	inline void _dump() { _waiters.dump(); }
private:
	ReadWriteWaiterList _waiters;
	int _wtype;
};


} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC_RW
