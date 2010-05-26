#ifndef OFLUX_LOCKFREE_ATOMIC
#define OFLUX_LOCKFREE_ATOMIC

#include "atomic/OFluxAtomic.h"
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

template<typename T>
inline T * unmk(T * t)
{
	return (is_val<0x0001>(t) ? NULL : t);
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


struct EventBaseHolder {
	enum WType 
		{ None = oflux::atomic::AtomicCommon::None
		, Read = oflux::atomic::AtomicReadWrite::Read
		, Write = oflux::atomic::AtomicReadWrite::Write
		, Exclusive = oflux::atomic::AtomicExclusive::Exclusive 
		};

	EventBaseHolder(  EventBasePtr & a_ev
			, int a_type = None)
		: ev(a_ev)
		, next(NULL)
		, type(a_type)
		, resource_loc(NULL)
		, resource(NULL)
	{}
	EventBaseHolder(  EventBasePtr & a_ev
			, EventBaseHolder ** a_resource_loc
			, int a_type = None)
		: ev(a_ev)
		, next(NULL)
		, type(a_type)
		, resource_loc(a_resource_loc)
		, resource(NULL)
	{}

	EventBaseHolder( void * a_resource )
		: ev()
		, next(NULL)
		, type(None)
		, resource_loc(NULL)
		, resource(a_resource)
	{}

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
		bool acqed = _waiters.push(ebh,wtype);
		if(acqed) {
			oflux_log_debug("RW::a_o_w %s %p %p acqed %d %d\n"
				, ev->flow_node()->getName()
				, ev.get()
				, this
				, wtype
				, _waiters.rcount);
			_wtype = wtype;
			//if(wtype == EventBaseHolder::Read) {
			allocator.put(ebh); 
			//}
		} else {
			oflux_log_debug("RW::a_o_w %s %p %p waited %d %d\n"
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
		oflux_log_debug("RW::rel   %s %p %p releasing %d %d\n"
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
			oflux_log_debug("RW::rel   %s %p %p came out %d %d\n"
				, e->ev->flow_node()->getName()
				, e->ev.get()
				, this
				, e->type
				, _waiters.rcount);
			//_wtype = e->type;
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
		: _head(new EventBaseHolder(NULL))
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
protected:
	PoolEventList waiters;
	AtomicPooled * head;
};

class AtomicPooled : public oflux::atomic::Atomic {
public:
	friend class AtomicPool;

	AtomicPooled(AtomicPool & pool, void * data);
	virtual ~AtomicPooled() {}
	virtual int held() const { return ! _pool.waiters.empty(); }
	virtual size_t waiter_count() { return _pool.waiters.count(); }
	virtual int wtype() const { return 0; }
	virtual bool acquire_or_wait(EventBasePtr & ev,int);
	virtual void release(
		  std::vector<EventBasePtr> & rel_ev
		, EventBasePtr & by_ev);
	virtual void ** data() { return &(_resource_ebh->resource); }
        virtual const char * atomic_class() const { return atomic_class_str; }
	virtual bool is_pool_like() const { return true; }
	virtual void relinquish() {}
protected:
	AtomicPooled * _next;
private:
	static const char * atomic_class_str;
	AtomicPool & _pool;
	EventBaseHolder * _resource_ebh;
	EventBaseHolder * _by_ebh;
};

} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC
