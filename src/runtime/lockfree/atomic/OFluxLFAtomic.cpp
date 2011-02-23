#include "lockfree/atomic/OFluxLFAtomic.h"
#include "lockfree/allocator/OFluxLFMemoryPool.h"
#include "lockfree/allocator/OFluxSMR.h"
#include "flow/OFluxFlowNode.h"
#include "OFluxLogging.h"

namespace oflux {
namespace lockfree {
namespace atomic {

Allocator<EventBaseHolder,DeferFree> AtomicCommon::allocator; //(new allocator::MemoryPool<sizeof(EventBaseHolder)>());

static const char *
convert_wtype_to_string(int wtype)
{
	static const char * conv[] =
		{ "None "
		, "Read "
		, "Write"
		, "Excl."
		, "Upgr."
		};
	static const char * fallthrough = "?    ";
	return (wtype >=0 && (size_t)wtype < (sizeof(conv)/sizeof(fallthrough))
		? conv[wtype]
		: fallthrough);
}

AtomicCommon::~AtomicCommon()
{}

void
AtomicCommon::_log_snapshot_waiters(const WaiterList * wlp)
{
	EventBaseHolder * e = wlp->_head;
	size_t repeat_count = 0;
	const char * this_wtype = NULL;
	const char * this_name = NULL;
	const char * last_wtype = "";
	const char * last_name = "";
	while(e && e != wlp->_tail && !is_one(e) && !is_three(e) && e->ev) {
		this_wtype = convert_wtype_to_string(e->type);
		this_name = e->ev->flow_node()->getName();
		if(this_wtype == last_wtype && this_name == last_name) {
			// no line emitted
		} else {
			if(repeat_count > 1) {
				oflux_log_trace("       ...(repeated %u times)\n"
					, repeat_count);
			}
			oflux_log_trace("   %s %s\n"
				, this_wtype
				, this_name);
			repeat_count = 0;
		}
		++repeat_count;
		last_wtype = this_wtype;
		last_name = this_name;
		e = e->next;
	}
	if(repeat_count > 1) {
		oflux_log_trace("       ...(repeated %u times)\n"
			, repeat_count);
	}
}

///////////////////////////////////////////////////////////////////////////////
// ExclusiveWaiterList state encoding:
//
// 1. (empty) head == tail, head->next == 0x0001 
// 2. (held0) head == tail, head->next == NULL
// 3. (heldM) head != tail, head->next > 0x0001 
//
// transitions:
// 2 -> 1 : cas head->next from NULL to 0x0001
// 1 -> 2 : cas head->next from 0x0001 to NULL
// 3 -> 2 : cas head       from head to head->next
// 2 -> 3 : cas tail->next from NULL to e
// (of course 3 -> 3 case is there for push and pop)

ExclusiveWaiterList::~ExclusiveWaiterList()
{
	EventBaseHolder * e = _head;
	EventBaseHolder * en;
	while(unmk(en = e->next)) {
		AtomicCommon::allocator.put(e);
		e = en;
	}
	if(unmk(e)) {
		AtomicCommon::allocator.put(e);
	}
}

bool
ExclusiveWaiterList::push(EventBaseHolder *e)
{
	bool res = false;
#ifdef LF_EX_WAITER_INSTRUMENTATION
	Observation & obs = log.submit();
	obs.tid = oflux_self();
	obs.h = 0;
	obs.hn = 0;
	obs.t = 0;
	obs.action = Observation::Action_A_o_w;
	obs.trans = "";
	obs.res = 0;
	obs.e = e;
	obs.cas_addr = 0;
	obs.cas_val = 0;
	obs.tail_up =-1;
	obs.ev = e->ev;
	obs.retries = 0;
	obs.term_index = 0;
#endif // LF_EX_WAITER_INSTRUMENTATION
	e->next = NULL;
	EventBaseHolder * t = NULL;
	EventBase * ev = NULL; // hold the event locally
	ev_swap(ev,e->ev);
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
	while(1) {
		//h = _head;
		HAZARD_PTR_ASSIGN(h,_head,0); // 
		hn = h->next;
#ifdef LF_EX_WAITER_INSTRUMENTATION
		obs.h = h;
		obs.hn = hn;
#endif // LF_EX_WAITER_INSTRUMENTATION
#ifdef LF_EX_WAITER_INSTRUMENTATION
#endif // LF_EX_WAITER_INSTRUMENTATION

		if(is_one(hn) 
				&& __sync_bool_compare_and_swap(
				  &(h->next)
				, 0x0001
				, NULL)) {
			// 1->2
			ev_swap(e->ev,ev);
			res = true; 
#ifdef LF_EX_WAITER_INSTRUMENTATION
			obs.cas_addr = &(h->next);
			obs.cas_val = NULL;
			obs.trans = "1->2";
#endif // LF_EX_WAITER_INSTRUMENTATION
			break;
		} else {
			// (2,3)->3
			//t = _tail;
			HAZARD_PTR_ASSIGN(t,_tail,1); // 
#ifdef LF_EX_WAITER_INSTRUMENTATION
			obs.t = t;
#endif // LF_EX_WAITER_INSTRUMENTATION
			if(unmk(t->next)) {
#ifdef LF_EX_WAITER_INSTRUMENTATION
				++obs.retries;
#endif // LF_EX_WAITER_INSTRUMENTATION
				continue;
			}
			if(unmk(t) && __sync_bool_compare_and_swap(
					  &(t->next)
					, NULL
					, e)) {

#ifdef LF_EX_WAITER_INSTRUMENTATION
				obs.cas_addr = &(t->next);
				obs.cas_val = e;
				obs.tail_up =
#endif // LF_EX_WAITER_INSTRUMENTATION
				__sync_bool_compare_and_swap(&_tail,t,e);
				ev_swap(t->ev,ev);
#ifdef LF_EX_WAITER_INSTRUMENTATION
				obs.t = t;
				obs.trans = "(2,3)->3";
#endif // LF_EX_WAITER_INSTRUMENTATION
				break;
			}
		}
#ifdef LF_EX_WAITER_INSTRUMENTATION
		++obs.retries;
#endif // LF_EX_WAITER_INSTRUMENTATION
	}
	HAZARD_PTR_RELEASE(0);
	HAZARD_PTR_RELEASE(1);
#ifdef LF_EX_WAITER_INSTRUMENTATION
	obs.res = res;
	obs.term_index = log.at();
	obs.action = Observation::Action_a_o_w;
#endif // LF_EX_WAITER_INSTRUMENTATION
	return res;
}

EventBaseHolder * 
ExclusiveWaiterList::pop()
{
#ifdef LF_EX_WAITER_INSTRUMENTATION
	Observation & obs = log.submit();
	obs.action = Observation::Action_Rel;
	obs.h = 0;
	obs.hn = 0;
	obs.t = 0;
	obs.tid = oflux_self();
	obs.trans = "";
	obs.res = 0;
	obs.e = 0;
	obs.tail_up =-1;
	obs.ev = 0;
	obs.term_index = 0;
	obs.retries = 0;
#endif // LF_EX_WAITER_INSTRUMENTATION
	EventBaseHolder * r = NULL;
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
	EventBaseHolder * t = NULL;
	while(1) {
		HAZARD_PTR_ASSIGN(h,_head,0); // 
		//h = _head;
		hn = h->next;
		HAZARD_PTR_ASSIGN(t,_tail,1); // 
		//t = _tail;
#ifdef LF_EX_WAITER_INSTRUMENTATION
		obs.h = h;
		obs.hn = hn;
		obs.t = t;
#endif // LF_EX_WAITER_INSTRUMENTATION
		if(unmk(t) && unmk(t->next)) {
#ifdef LF_EX_WAITER_INSTRUMENTATION
			++obs.retries;
#endif // LF_EX_WAITER_INSTRUMENTATION
			__sync_synchronize();
			continue;
		}
		if(h != _tail && hn != NULL 
				&& !is_one(hn)
				&& h->ev != NULL
				&& __sync_bool_compare_and_swap(
					  &_head
					, h
					, hn)) {
			// 3->2
#ifdef LF_EX_WAITER_INSTRUMENTATION
			obs.cas_addr = &_head;
			obs.cas_val = hn;
			obs.trans = "3->2";
#endif // LF_EX_WAITER_INSTRUMENTATION
			r = h;
			r->next = NULL;
#ifdef LF_EX_WAITER_INSTRUMENTATION
			obs.e = r;
			obs.ev = r->ev;
#endif // LF_EX_WAITER_INSTRUMENTATION
			break;
		} else if(hn==NULL && __sync_bool_compare_and_swap(
				  &(h->next)
				, hn
				, 0x0001)) {
			// 2->1
#ifdef LF_EX_WAITER_INSTRUMENTATION
			obs.cas_addr = &(h->next);
			obs.cas_val = reinterpret_cast<const EventBaseHolder *> (0x0001);
			obs.trans = "2->1";
#endif // LF_EX_WAITER_INSTRUMENTATION
			break; //empty
		}
#ifdef LF_EX_WAITER_INSTRUMENTATION
		++obs.retries;
#endif // LF_EX_WAITER_INSTRUMENTATION
	}
	HAZARD_PTR_RELEASE(0);
	HAZARD_PTR_RELEASE(1);
	if(r) r->busyWaitOnEv();
#ifdef LF_EX_WAITER_INSTRUMENTATION
	obs.res = (r!=0);
	obs.term_index = log.at();
	obs.action = Observation::Action_rel;
#endif // LF_EX_WAITER_INSTRUMENTATION
	return r;
}

void
ExclusiveWaiterList::dump()
{
        const char * state =
                (is_one(_head->next) ?
                        "empty [1]"
                : (_head->next == NULL ?
                        "held0 [2]" :
                        "heldM [3]"));

        oflux_log_trace(" atomic[%p] in state %s\n"
                , this
                , state);
	char buff[5000];
	size_t at = 0;
        at += snprintf(buff+at,5000-at,"  head = ");
        EventBaseHolder * e = _head;
        while(e != NULL && !is_one(e)) {
                at += snprintf(buff+at,5000-at,"%p->",e->ev);
                e  = e->next;
        }
        if(e == NULL) {
                at += snprintf(buff+at,5000-at,"(0x0)\n");
        } else if(is_one(e)) {
                at += snprintf(buff+at,5000-at,"(0x1)\n");
        } else {
                at += snprintf(buff+at,5000-at,"\n");
        }
	oflux_log_trace("%s",buff);
        oflux_log_trace("  tail = %p\n",_tail->ev);
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux

