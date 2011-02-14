#ifndef OFLUX_LOCKFREE_ATOMIC_POOLED
#define OFLUX_LOCKFREE_ATOMIC_POOLED

#include "OFluxLFAtomic.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "OFluxRollingLog.h"
#include "OFluxGrowableCircularArray.h"

namespace oflux {
namespace lockfree {
namespace atomic {


class PoolEventList {
public:
	typedef oflux::lockfree::growable::TStructEntry<EventBaseHolder> EBHolderEntry;

	PoolEventList()
		: _ev_q_out(0)
		, _rs_q_out(0)
	{
		_uin.u64 = 0ULL;
		oflux_log_info("PoolEventList() %p has _rs_q._impl of %p\n"
			, this
			, _rs_q._impl);
	}

	void dump() { /*TBD*/ }

	inline bool acquire_or_wait(EventBaseHolder * ev)
	{
		bool res = false;
		long rs_q_out;
		UIn uin;
		UIn n_uin;
		EventBaseHolder * resource = NULL;
		EBHolderEntry * ebptr;
		EBHolderEntry * ebholderptr;
		long retries = 0;
		while(1) {
			rs_q_out = _rs_q_out;
			uin.u64 = _uin.u64;
			n_uin.u64 = uin.u64;
			++n_uin.s._ev_q_in;
			if((_uin.s._ev_q_in - _ev_q_out) <= 0
				&& (_uin.s._rs_q_in - _rs_q_out) > 0
				&& (ebholderptr = _rs_q.get(rs_q_out,false))
				&& (resource = ebholderptr->ptr)
				&& resource != NULL
				&& ebholderptr->at >= 0
				// pop a resource
				&& __sync_bool_compare_and_swap(
					& _rs_q_out
					, rs_q_out
					, rs_q_out+1
				)) {
				bool cas_res = _rs_q.cas_to_null(rs_q_out,resource);
				assert(cas_res && "must write a NULL on an rs pop");
				*(ev->resource_loc) = resource;
				res = true;
				break;
			} else if((_uin.s._rs_q_in - rs_q_out) <= 0
				&& (uin.s._ev_q_in - _ev_q_out +1 >= _ev_q.impl_size() ? _ev_q.grow() : 1)
				&& (ebptr = _ev_q.get(uin.s._ev_q_in))
				&& !ebptr->ptr
				// push ev
				&& __sync_bool_compare_and_swap(
					& _uin.u64
					, uin.u64
					, n_uin.u64
				)) {
				bool cas_res = _ev_q.cas_from_null(uin.s._ev_q_in, ev);
				assert(cas_res && "must write on ev push");
				res = false;
				break;
			}
			++retries;
		}
		return res;
	}
	inline EventBaseHolder * release(EventBaseHolder * by_ev)
	{
		EventBaseHolder * res = NULL;
		EventBaseHolder * ev_out = NULL;
		long ev_q_out;
		UIn uin;
		UIn n_uin;
		EBHolderEntry * ebptr;
		EBHolderEntry * ebholderptr;
		EventBaseHolder * resource = *(by_ev->resource_loc);
		*(by_ev->resource_loc) = NULL;
		long retries = 0;
		while(1) {
			ev_q_out = _ev_q_out;
			uin.u64 = _uin.u64;
			n_uin.u64 = uin.u64;
			++n_uin.s._rs_q_in;
			
			if((_uin.s._ev_q_in - ev_q_out) > 0
				&& (ebptr = _ev_q.get(ev_q_out,false))
				&& (ev_out = ebptr->ptr)
				&& ev_out != NULL
				&& ebptr->at >= 0
				// pop on ev_q
				&& __sync_bool_compare_and_swap(
					& _ev_q_out
					, ev_q_out
					, ev_q_out+1
					)) {
				bool cas_res = _ev_q.cas_to_null(ev_q_out,ev_out);
				assert(cas_res && "must write NULL on ev pop");
				*(ev_out->resource_loc) = resource;
				res = ev_out;
				break;
			} else if((_uin.s._ev_q_in - ev_q_out) <= 0
				// grow if needed
				&& (uin.s._rs_q_in - _rs_q_out+1 >= _rs_q.impl_size() ? _rs_q.grow() : 1)
				&& (ebholderptr = _rs_q.get(uin.s._rs_q_in))
				&& !ebholderptr->ptr
				
				// push resource
				&& __sync_bool_compare_and_swap(
					& _uin.u64
					, uin.u64
					, n_uin.u64
					)) {
				bool cas_res = _rs_q.cas_from_null(uin.s._rs_q_in,resource);
				assert(cas_res && "must write on an rs push");
				res = NULL;
				break;
			}
			++retries;
		}
		return res;
	}
	size_t waiter_count() const
	{ return static_cast<size_t>(_uin.s._ev_q_in - _ev_q_out); }

private:
	PoolEventList(const PoolEventList &); // not implemented
private:
	oflux::lockfree::growable::CircularArray<EventBaseHolder> _ev_q;
	oflux::lockfree::growable::CircularArray<EventBaseHolder> _rs_q;
	volatile long _ev_q_out;
	volatile long _rs_q_out;
	volatile union UIn { 
		uint64_t u64; 
		struct S { long _ev_q_in; long _rs_q_in; } s; 
		} _uin;
};

class AtomicPooled;

class AtomicPool : public oflux::atomic::AtomicMapAbstract {
public:
	friend class AtomicPooled;

	static Allocator<AtomicPooled,DeferFree> allocator;

	AtomicPool();
	virtual ~AtomicPool();
	virtual void * new_key() const { return NULL; }
	virtual void delete_key(void *) const {}
	virtual const void * get(oflux::atomic::Atomic * & a_out,const void * key);
	virtual oflux::atomic::AtomicMapWalker * walker();
	virtual int compare (const void * v_k1, const void * v_k2) const
	{ return 0; /* == always */ }
	void release(AtomicPooled *ap);
	void _dump() { waiters->dump(); }
	static void dump(oflux::atomic::AtomicMapAbstract * map)
	{ reinterpret_cast<AtomicPool *>(map)->_dump(); }
protected:
	PoolEventList * waiters;
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
		_by_ebh->ev = NULL;
		resource_loc_asgn(_by_ebh,&_resource_ebh);
		oflux_log_trace2("APD::init() APD*this:%p\n",this);
		_by_ebh->resource_loc = &_resource_ebh;
		//check();
	}
	void check();
	virtual int held() const 
	{ return _pool->waiters->waiter_count() > 0; }
	virtual size_t waiter_count() 
	{ return _pool->waiters->waiter_count(); }
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
	EventBaseHolder * _by_ebh;
};


} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC_POOLED
