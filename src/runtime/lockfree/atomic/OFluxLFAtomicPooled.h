#ifndef OFLUX_LOCKFREE_ATOMIC_POOLED
#define OFLUX_LOCKFREE_ATOMIC_POOLED
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

#include "OFluxLFAtomic.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "OFluxRollingLog.h"
#include "OFluxGrowableCircularArray.h"
#include "lockfree/allocator/OFluxSMR.h"

namespace oflux {
namespace lockfree {
namespace atomic {

class AtomicPooledBase : public oflux::atomic::Atomic {
public:
	void * _data;
};

class PoolEventList {
public:
	typedef oflux::lockfree::growable::TStructEntry<AtomicPooledBase> APBHolderEntry;
	typedef oflux::lockfree::growable::TStructEntry<void> ResourceHolderEntry;


	typedef uint32_t index_t; 


	PoolEventList()
		: _ev_q_out(0)
		, _rs_q_out(0)
	{
		_uin.u64 = 0ULL;
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] "
			  "PoolEventList() %p has _rs_q._impl of %p\n"
			, oflux_self()
			, this
			, _rs_q._impl);
	}

	void dump() { /*TBD*/ }

	inline bool acquire_or_wait(AtomicPooledBase * ev)
	{
		::oflux::lockfree::smr::set(ev,0);
		bool res = false;
		index_t rs_q_out;
		UIn uin;
		UIn n_uin;
		void * resource = NULL;
		APBHolderEntry * apbptr;
		ResourceHolderEntry * rsholderptr;
		long retries = 0;
		ev->_data = NULL;
		oflux::lockfree::load_load_barrier();
		while(1) {
			rs_q_out = _rs_q_out;
			uin.u64 = _uin.u64;
			n_uin.u64 = uin.u64;
			++n_uin.s._ev_q_in;
			if((_uin.s._ev_q_in - _ev_q_out) <= 0
				&& (_uin.s._rs_q_in - _rs_q_out) > 0
				&& (rsholderptr = _rs_q.get(rs_q_out,false))
				&& (resource = rsholderptr->ptr)
				&& resource != NULL
				&& rsholderptr->at >= 0
				// pop a resource
				&& __sync_bool_compare_and_swap(
					& _rs_q_out
					, rs_q_out
					, rs_q_out+1
				)) {
				const bool cas_res = _rs_q.cas_to_null(rs_q_out,resource);
				(void)cas_res;
				assert(cas_res && "must write a NULL on an rs pop");
				const bool data_cas_res = 
					__sync_bool_compare_and_swap(
						& ev->_data
						, NULL
						, resource);
				(void)data_cas_res;
				assert(data_cas_res && "must cas ap_out._data from NULL on rs pop");
				res = true;
				break;
			} else if((_uin.s._rs_q_in - rs_q_out) <= 0
				&& (uin.s._ev_q_in - _ev_q_out +1 >= _ev_q.impl_size() ? _ev_q.grow() : 1)
				&& (apbptr = _ev_q.get(uin.s._ev_q_in))
				&& !apbptr->ptr
				// push ev
				&& __sync_bool_compare_and_swap(
					& _uin.u64
					, uin.u64
					, n_uin.u64
				)) {
				const bool cas_res = _ev_q.cas_from_null(uin.s._ev_q_in, ev);
				(void)cas_res;
				assert(cas_res && "must write on ev push");
				res = false;
				break;
			}
			++retries;
		}
		HAZARD_PTR_RELEASE(0)
		return res;
	}
	inline AtomicPooledBase * release(AtomicPooledBase * by_ev)
	{
		AtomicPooledBase * res = NULL;
		AtomicPooledBase * ev_out = NULL;
		index_t ev_q_out;
		UIn uin;
		UIn n_uin;
		APBHolderEntry * apbptr;
		ResourceHolderEntry * rsholderptr;
		void * resource = by_ev->_data;
		by_ev->_data = NULL;
		long retries = 0;
		while(1) {
			ev_q_out = _ev_q_out;
			uin.u64 = _uin.u64;
			n_uin.u64 = uin.u64;
			++n_uin.s._rs_q_in;
			
			if((_uin.s._ev_q_in - ev_q_out) > 0
				&& (apbptr = _ev_q.get(ev_q_out,false))
				&& (ev_out = apbptr->ptr)
				&& ev_out != NULL
				&& apbptr->at >= 0
				// pop on ev_q
				&& __sync_bool_compare_and_swap(
					& _ev_q_out
					, ev_q_out
					, ev_q_out+1
					)) {
				const bool cas_res = _ev_q.cas_to_null(ev_q_out,ev_out);
				(void)cas_res;
				assert(cas_res && "must write NULL on ev pop");
				const bool ev_cas_res =
					__sync_bool_compare_and_swap(
						& (ev_out->_data)
						, NULL
						, resource);
				(void)ev_cas_res;
				assert(ev_cas_res && "must cas ev._data from NULL on an ev pop");
				res = ev_out;
				break;
			} else if((_uin.s._ev_q_in - ev_q_out) <= 0
				// grow if needed
				&& (uin.s._rs_q_in - _rs_q_out+1 >= _rs_q.impl_size() ? _rs_q.grow() : 1)
				&& (rsholderptr = _rs_q.get(uin.s._rs_q_in))
				&& !rsholderptr->ptr
				
				// push resource
				&& __sync_bool_compare_and_swap(
					& _uin.u64
					, uin.u64
					, n_uin.u64
					)) {
				const bool cas_res = _rs_q.cas_from_null(uin.s._rs_q_in,resource);
				(void)cas_res;
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
	oflux::lockfree::growable::CircularArray<AtomicPooledBase> _ev_q;
	oflux::lockfree::growable::CircularArray<void> _rs_q;
	volatile index_t _ev_q_out;
	volatile index_t _rs_q_out;
	volatile union UIn { 
		uint64_t u64; 
		struct S { index_t _ev_q_in; index_t _rs_q_in; } s; 
		} __attribute__((__packed__))_uin;
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
};

class AtomicPooled : public AtomicPooledBase {
public:
	friend class AtomicPool;

	AtomicPooled(AtomicPool * pool, void * data);
	virtual ~AtomicPooled() 
	{
		oflux_log_debug("[" PTHREAD_PRINTF_FORMAT "] "
			"~AtomicPooled %p\n"
			, oflux_self()
			, this);
	}
	inline void init()
	{
		_next = NULL;
		_by_ev = NULL;
		oflux::lockfree::store_load_barrier();
		oflux_log_trace2("APD::init() APD*this:%p\n",this);
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
	{ return & _data; }
        virtual const char * atomic_class() const { return atomic_class_str; }
	virtual bool is_pool_like() const { return true; }
	virtual void relinquish(bool);
	virtual bool can_relinquish() const { return true; }
protected:
	AtomicPooled * _next;
private:
	static const char * atomic_class_str;
	AtomicPool * _pool;
	EventBase * _by_ev;
};


} // namespace atomic
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_ATOMIC_POOLED
