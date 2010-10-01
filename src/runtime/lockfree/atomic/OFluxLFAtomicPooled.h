#ifndef OFLUX_LOCKFREE_ATOMIC_POOLED
#define OFLUX_LOCKFREE_ATOMIC_POOLED

#include "OFluxLFAtomic.h"
#include "lockfree/OFluxMachineSpecific.h"

namespace oflux {
namespace lockfree {
namespace atomic {



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
		EventBaseHolder * r_ebh;
		while(!(r_ebh = _resource_ebh)) {
			store_load_barrier();
			sched_yield();
		}
		return &(r_ebh->resource); 
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

#endif // OFLUX_LOCKFREE_ATOMIC_RW
