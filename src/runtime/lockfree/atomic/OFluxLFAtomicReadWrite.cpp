#include "OFluxLFAtomicReadWrite.h"
#include <cstdio>

namespace oflux {
namespace lockfree {
namespace atomic {

Allocator<readwrite::EventBaseHolder> ReadWriteWaiterList::allocator; //(new allocator::MemoryPool<sizeof(EventBaseHolder)>());


/////////////////////////////////////////////////////////////////////////////
// ReadWriteWaiterList States:
//
// rcount indicates the number of holders
//  mode: true/1 means read, false/0 means write
//
// 1. Empty
//    head == tail; head.next = 0x0000
// 2. HeldNoWaiters
//    head == tail; 
//    head.next = 0x0001 
//        | (read_mode ? 0x0002 : 0)
//        | (rcount << 2)
// 3. HeldSomeWaiters
//    head != tail;
//    head.next is valid ptr: (lsb is 0) and != 0x0000

ReadWriteWaiterList::~ReadWriteWaiterList()
{
	readwrite::EventBaseHolder * e = _head;
	readwrite::EventBaseHolder * en;
	while((en = e->next.ptr()) && !e->next.mkd()) {
		allocator.put(e);
		e = en;
	}
	if(e) {
		allocator.put(e);
	}
}

bool
ReadWriteWaiterList::push(readwrite::EventBaseHolder * e)
{
	bool res = false;
#ifdef LF_RW_WAITER_INSTRUMENTATION
	Observation & obs = log.submit();
	obs.tid = pthread_self();
	obs.action = Observation::Action_A_o_w;
	obs.trans = "";
	obs.res = 0;
	obs.e = e;
	obs.ev = e->val.get();
	obs.mode = e->mode;
	obs.retries = 0;
	obs.term_index = 0;
#endif // LF_RW_WAITER_INSTRUMENTATION
	SafeEventBasePtr ev;
	ev.swap(e->val);
	assert(!e->val.get());
	int mode = e->mode;
	while(1) {
		readwrite::EventBaseHolder * h = _head;
		readwrite::EventBaseHolder * t = _tail;
		RWWaiterPtr rwptr(t->next);
#ifdef LF_RW_WAITER_INSTRUMENTATION
		obs.u64=rwptr.u64();
#endif // LF_RW_WAITER_INSTRUMENTATION
		if(        h==t 
			&& !rwptr.ptr()
			&& t->next.compareAndSwap(
				  rwptr
				, 1 // count
				, e->mode
				, 1 // mkd
				, rwptr.epoch())
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "1->2";
#endif // LF_RW_WAITER_INSTRUMENTATION
			res = true; // acquire
			e->val.swap(ev);
			e->mode = mode;
			assert(!ev.get());
			break;
		} else if( h==t 
			&& rwptr.mkd()
			&& rwptr.mode() // read
			&& e->mode
			&& t->next.compareAndSwap(
				  rwptr
				, rwptr.rcount()+1
				, true // mode (read)
				, true // mkd
				, rwptr.epoch()+1)
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "2->2";
#endif // LF_RW_WAITER_INSTRUMENTATION
			res = true; // acquire
			e->val.swap(ev);
			e->mode = mode;
			assert(!ev.get());
			break;
		} else if( h==t
			&& rwptr.mkd()
			&& !(e->mode && rwptr.mode())
			&& e->next.set(
				  rwptr.rcount()
				, rwptr.mode()
				, true
				, rwptr.epoch())
			&& t->next.compareAndSwap(
				  rwptr
				, e
				, rwptr.epoch()+1)
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "2->3";
#endif // LF_RW_WAITER_INSTRUMENTATION
			_tail = e;
			assert(t->val.get() == 0);
			t->mode = mode;
			t->val.swap(ev);
			assert(ev.get() == 0);
			res = false; // wait
			break;
		} else if( h!=t
			&& !h->next.mkd()
			&& rwptr.mkd()
			&& e->next.set(
				  rwptr.rcount()
				, rwptr.mode()
				, true
				, rwptr.epoch())
			&& t->next.compareAndSwap(
				  rwptr
				, e
				, rwptr.epoch()+1)
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "3->3";
#endif // LF_RW_WAITER_INSTRUMENTATION
			_tail = e;
			t->mode = mode;
			t->val.swap(ev);
			res = false; // wait
			break;
		}
#ifdef LF_RW_WAITER_INSTRUMENTATION
		++obs.retries;
#endif // LF_RW_WAITER_INSTRUMENTATION
	}
#ifdef LF_RW_WAITER_INSTRUMENTATION
	obs.res = res;
	obs.term_index = log.at();
	obs.action = Observation::Action_a_o_w;
#endif // LF_RW_WAITER_INSTRUMENTATION
	return res;
}

namespace readwrite {
inline void busyWaitOnEv(const EventBasePtr & ev)
{
	size_t retries = 0;
	size_t warning_level = 1;
	while(ev.get() == NULL) {
		if(retries > warning_level) {
			oflux_log_error("readwrite::busyWaitOnEv retries at %d\n", retries);
			warning_level = std::max(warning_level, warning_level << 1);
		}
		sched_yield();
		store_load_barrier();
		++retries;
	}
}
} // namespace readwrite

void
ReadWriteWaiterList::pop(
	  readwrite::EventBaseHolder * & el
	, const EventBasePtr & by_ev
	, int mode)
{
#ifdef LF_RW_WAITER_INSTRUMENTATION
	Observation & obs = log.submit();
	obs.action = Observation::Action_Rel;
	obs.tid = pthread_self();
	obs.trans="";
	obs.res=0;
	obs.e = 0;
	obs.mode = mode;
	obs.ev = by_ev.get();
	obs.retries=0;
#endif // LF_RW_WAITER_INSTRUMENTATION
	el = NULL;
	while(1) {
		readwrite::EventBaseHolder * h = _head;
		readwrite::EventBaseHolder * t = _tail;
		RWWaiterPtr rwptr(t->next);
#ifdef LF_RW_WAITER_INSTRUMENTATION
		obs.u64 = rwptr.u64();
#endif // LF_RW_WAITER_INSTRUMENTATION
		if(!rwptr.mkd()) {
			continue;
		}
		assert(mode == rwptr.mode()
			&& "release mode should match waiter state");
		if(        h==t
			/*&& rwptr.mkd()*/
			&& rwptr.rcount()==1
			&& t->next.compareAndSwap(
				  rwptr
				, 0 // count
				, false // mode
				, false
				, rwptr.epoch()+1) // mkd
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "2->1";
#endif // LF_RW_WAITER_INSTRUMENTATION
			el = NULL;
			break;
		} else if( /* rwptr.mkd()*/
			   rwptr.rcount() > 1
			&& t->next.compareAndSwap(
				  rwptr
				, rwptr.rcount()-1
				, rwptr.mode()
				, rwptr.mkd()
				, rwptr.epoch()+1)
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "(2,3)->(2,3)";
#endif // LF_RW_WAITER_INSTRUMENTATION
			el = NULL;
			break;
		} else if( h!=t
			&& h->val
			&& h->next.ptr()
			&& !h->next.mkd()
			/*&& rwptr.mkd()*/
			&& rwptr.rcount() == 1
			) {
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "3->(2,3)";
#endif // LF_RW_WAITER_INSTRUMENTATION
			el = h;
			int new_rcount = 1;
			while(h->next.ptr() 
				&& el->mode // read
				&& h->next.ptr()->mode // read
				&& h->val
				&& !h->next.ptr()->next.mkd()) {
				h = h->next.ptr();
				++new_rcount;
			}
			if( __sync_bool_compare_and_swap(&_head,el,h->next.ptr())) {
				while(t!=_tail || !t->next.compareAndSwap(
					  rwptr
					, new_rcount
					, el->mode
					, true
					, rwptr.epoch()+1)
					) {
					t = _tail;
					rwptr = t->next;
				}
				h->next.set(0,0);
#ifdef LF_RW_WAITER_INSTRUMENTATION
				obs.res = new_rcount;
				obs.e = el;
				obs.ev = el->val.get();
#endif // LF_RW_WAITER_INSTRUMENTATION
				break;
			}
#ifdef LF_RW_WAITER_INSTRUMENTATION
			obs.trans = "";
#endif // LF_RW_WAITER_INSTRUMENTATION
			// continuing here
		}
#ifdef LF_RW_WAITER_INSTRUMENTATION
		++obs.retries;
#endif // LF_RW_WAITER_INSTRUMENTATION
	}
	readwrite::EventBaseHolder * el_traverse = el;
	while(el_traverse) {
		readwrite::busyWaitOnEv(el_traverse->val);
		el_traverse = el_traverse->next.ptr();
	}
#ifdef LF_RW_WAITER_INSTRUMENTATION
	obs.term_index = log.at();
	obs.action = Observation::Action_rel;
#endif // LF_RW_WAITER_INSTRUMENTATION
}

void
ReadWriteWaiterList::dump()
{
        // todo at oflux_log_trace()
	readwrite::EventBaseHolder * h = _head;
	readwrite::EventBaseHolder * t = _tail;
	RWWaiterPtr rwptr(t->next);
	const char * state =
		(h == t
			? (rwptr.mkd() ? 
				"empty           [1]"
				:
				"heldNoWaiters   [2]")
			:       "heldSomeWaiters [3]");
	oflux_log_trace(" atomic[%p] in state %s\n"
		, this
		, state);
	char buff[5000];
	size_t at = 0;
	at += snprintf(buff+at,5000-at,"  head = ");
}

size_t
ReadWriteWaiterList::count_waiters() const
{
	readwrite::EventBaseHolder * e = _head;
	size_t res = 0;
	while(e && e->next.ptr() && !e->next.mkd()) {
		e = e->next.ptr();
		++res;
	}
	return res;
}

bool
ReadWriteWaiterList::has_waiters() const
{
	readwrite::EventBaseHolder * e = _head;
	return e && e->next.ptr() && !e->next.mkd();
}

bool
ReadWriteWaiterList::held() const
{
	readwrite::EventBaseHolder * e = _tail;
	return e->next.mkd();
}

size_t
ReadWriteWaiterList::rcount() const
{
	while(1) {
		readwrite::EventBaseHolder * t = _tail;
		RWWaiterPtr rwptr(t->next);
		if(_head == t && !rwptr.mkd()) {
			return 0;
		} else if(rwptr.mkd()) {
			return rwptr.rcount();
		}
	}
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux
