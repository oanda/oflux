#ifndef OFLUX_LOCKFREE_ATOMIC
#define OFLUX_LOCKFREE_ATOMIC

#include "atomic/OFluxAtomic.h"
#include "event/OFluxEventBase.h"
#include "boost/shared_ptr.hpp"
#include "OFluxAllocator.h"

//#include "OFlux.h"
//#include "OFluxLogging.h"
//#include "flow/OFluxFlowNode.h"

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
private:
	ExclusiveWaiterList _waiters;
};

class ReadWriteWaiterList : public WaiterList {
public:
	ReadWriteWaiterList() : rcount(0) {}

	bool push(EventBaseHolder * e, int type);
	void pop(EventBaseHolder * & el, int by_type);
	void dump();
	
	int rcount;
};

class AtomicReadWrite : public AtomicCommon {
public:
	AtomicReadWrite(void * data)
		: AtomicCommon(data)
		, _wtype(EventBaseHolder::None)
	{}
	virtual ~AtomicReadWrite() {}
	virtual int held() const 
	{ return _waiters.rcount ? _waiters.rcount : !_waiters.empty(); }
	virtual size_t waiter_count() { return _waiters.count(); }
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
			: wtype)
		bool acqed = _waiters.push(
			  ebh
			, local_wtype);
		if(acqed) {
			oflux_log_trace2("RW::a_o_w %s %p %p acqed %d %d\n"
				, ev->flow_node()->getName()
				, ev.get()
				, this
				, wtype
				, _waiters.rcount);
			_wtype = local_wtype;
			AtomicCommon::allocator.put(ebh); 
		} else {
			oflux_log_trace2("RW::a_o_w %s %p %p waited %d %d\n"
				, ev->flow_node()->getName()
				, ev.get()
				, this
				, wtype
				, _waiters.rcount);
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
			, _waiters.rcount);
		_waiters.pop(el,_wtype);
		//_wtype = EventBaseHolder::None;
		EventBaseHolder * e = el;
		EventBaseHolder * n_e = NULL;
		while(e) {
			n_e = e->next;
			_wtype = e->type;
			oflux_log_trace2("RW::rel   %s %p %p came out %d %d\n"
				, e->ev->flow_node()->getName()
				, e->ev.get()
				, this
				, e->type
				, _waiters.rcount);
			//_wtype = e->type;
			rel_ev.push_back(e->ev);
			AtomicCommon::allocator.put(e);
			e = n_e;
		}
	}
	virtual void log_snapshot_waiters() const;
private:
	ReadWriteWaiterList _waiters;
	int _wtype;
};



template<typename T>
class StampedPtr {
public:
	StampedPtr(T * tptr = NULL)
	{ 
		_content.s.stamp = 0;
		_content.s.ptr = tptr;
	}

	StampedPtr(const StampedPtr<T> & sp)
	{
		_content.s = sp._content.s;
	}

	StampedPtr<T> & operator=(const StampedPtr<T> & sp)
	{
		_content.s = sp._content.s;
		return *this;
	}

	inline int & stamp() { return _content.s.stamp; }

	inline T * & get() { return _content.s.ptr; }
	inline T * const & get() const { return _content.s.ptr; }

	bool cas(const StampedPtr<T> & old_sp,T * new_ptr)
	{ 
		union { uint64_t _uint; S s; } new_content;
		new_content.s.stamp = old_sp._content.s.stamp+1; 
		new_content.s.ptr = new_ptr;
		return __sync_bool_compare_and_swap(
			  &(_content._uint)
			, old_sp._content._uint
			, new_content._uint);
	}
private:
	struct S {
		int stamp;
		T * ptr;
	};
	union { 
		uint64_t _uint; // for alignment
		S s;
	} _content;
};

class PoolEventList { // thread safe
public:
	PoolEventList() 
		: _head(AtomicCommon::allocator.get()) //new EventBaseHolder(NULL))
		, _tail(_head.get())
	{}
	~PoolEventList();
	bool push(EventBaseHolder *e); // acquire a pool item
	EventBaseHolder * pop(EventBaseHolder * r); //release a pool item

	inline bool empty() const 
	{ 
		const EventBaseHolder * hp = _head.get();
		return !mkd(hp) && (hp->next == NULL);
	}

	inline size_t count() const
	{
		const EventBaseHolder * hp = _head.get();
		size_t res = 0;
		while(!mkd(hp) && hp->ev.get()) {
			if(hp->ev.get()) {
				++res;
			}
			hp = hp->next;
		}
		return res;
	}
	void dump();

private:
	EventBaseHolder * complete_pop();

	StampedPtr<EventBaseHolder> _head;
	EventBaseHolder * _tail;
};

class AtomicPooled;

class AtomicPool : public oflux::atomic::AtomicMapAbstract {
public:
	friend class AtomicPooled;

	static Allocator<AtomicPooled> allocator;

	AtomicPool();
	virtual ~AtomicPool();
	virtual void * new_key() const { return NULL; }
	virtual void delete_key(void *) const {}
	virtual const void * get(oflux::atomic::Atomic * & a_out,const void * key);
	virtual oflux::atomic::AtomicMapWalker * walker();
	virtual int compare (const void * v_k1, const void * v_k2) const
	{ return 0; /* == always */ }
	void release(AtomicPooled *ap);
	void _dump() { waiters.dump(); }
	static void dump(oflux::atomic::AtomicMapAbstract * map)
	{ reinterpret_cast<AtomicPool *>(map)->_dump(); }
protected:
	PoolEventList waiters;
	AtomicPooled * head_free;
};

class AtomicPooled : public oflux::atomic::Atomic {
public:
	friend class AtomicPool;

	AtomicPooled(AtomicPool * pool, void * data);
	virtual ~AtomicPooled() 
	{
		oflux_log_debug("~AtomicPooled %p\n",this);
	}
	inline void init()
	{
		_next = NULL;
		_by_ebh->ev.reset();
		resource_loc_asgn(_by_ebh,&_resource_ebh);
		oflux_log_trace2("APD::init() APD*this:%p\n",this);
		_by_ebh->resource_loc = &_resource_ebh;
		//check();
	}
	void check();
	virtual int held() const { return ! _pool->waiters.empty(); }
	virtual size_t waiter_count() { return _pool->waiters.count(); }
	virtual int wtype() const { return 0; }
	virtual bool acquire_or_wait(EventBasePtr & ev,int);
	virtual void release(
		  std::vector<EventBasePtr> & rel_ev
		, EventBasePtr & by_ev);
	virtual void ** data() 
	{ 
		assert(_resource_ebh);
		return &(_resource_ebh->resource); 
	}
        virtual const char * atomic_class() const { return atomic_class_str; }
	virtual bool is_pool_like() const { return true; }
	virtual void relinquish();
protected:
	AtomicPooled * _next;
private:
	static const char * atomic_class_str;
	AtomicPool * _pool;
	EventBaseHolder * _resource_ebh;
	// ownership of _resource_ebh based on state of this object:
	// STATE 0: constructed or fresh from _pool->head_free
	//  _resource_ebh
	//         points to EventBaseHolder {
	//                 ev = NULL
	//                 next = NULL
        //                 resource_loc = NULL
        //                 resource = ? (could be assigned from get()..data())
	//         }
	//  can release() --> STATE 1
	//  can acquire_or_wait() success --> STATE 2
	//  can acquire_or_wait() failure --> STATE 3
	// STATE 1: released
	//  old _resource_ebh belongs to _pool or to rel_ebh (ret from pop())
	//  _resource_ebh = allocator.get() // new one is had
	//  pool->release(this) called (here or in relinquish elsewhere)
	//  [ AtomicPooled ontop of rel_ebh was in STATE 3 with
	//    its own _resource_ebh == NULL, but now == this atomic's passed
	//    non-NULL value.
	//  ]
	//  --> STATE 0
	// STATE 2: acquired
	//  _resource_ebh reclaimed by allocator.put() and set to NULL
	//  _pool->waiters.push() ret true
	//   side-effect: _resource_ebh is written with a populated
	//         (resource != NULL) item
	//         so pool is giving that item up
	//  can release() --> STATE 1
	// STATE 3: waited
	//  _resource_ebh reclaimed by allocator.put() and set to NULL
	//  _pool->waiters.push() ret false
	//  [ when another AtomicPooled does STATE 1 transition and hits this,
	//     _resource_ebh goes to non-NULL
	//  ]
	//  release() --> STATE 1

	EventBaseHolder * _by_ebh;
	// ownership of _by_ebh based on state of this object:
	// STATE 0: constructed or fresh from _pool->head_free
	//  _by_ebh
	//         points to EventBaseHolder {
	//                 ev = ? // may be NULL if from a GuardInserter
	//                 next = NULL
        //                 resource_loc = &_resource_ebh
        //                 resource = NULL
	//         }
	//  can release() --> STATE 1
	//  can acquire_or_wait() success --> STATE 2
	//  can acquire_or_wait() failure --> STATE 3
	// STATE 1: released
	//  *(_by_ebh->resource_loc) = NULL so _resource_ebh = NULL
	//    [ the released rel_ebh has a non-NULL *(rel_ebh->resource_loc)
	//      so that AtomicPooled will have a non-NULL _resource_ebh
	//	his _by_ebh is _not_ related at all to rel_ebh
	//    ]
	//  if rel_ebh non-NULL then strip out its ev
	//     and reclaim it with allocator.put()
	//  --> STATE 0
	// STATE 2: acquired
	//  _by_ebh.ev = ev
	//  _by_ebh was not really used so no need for a new one
	//  can release() --> STATE 1
	// STATE 3: wait
	//  _by_ebh.ev = ev
	//  _by_ebh is given up to _pool since it is waiting
	//  _by_ebh = allocator.get()
	//  [ when another AtomicPooled does STATE 1, we'll
	//    get a _resource_ebh that is non-NULL out of it
	//    the local _by_ebh is not involved tho, and the given up
	//    one may not be directly used since the lockfree content
	//    swap may have happened (PoolEventList::push() for detail)
	//  ]
	// release() --> STATE 1
};

} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC
