#include "lockfree/atomic/OFluxLFAtomic.h"
#include "lockfree/allocator/OFluxLFMemoryPool.h"
#include "flow/OFluxFlowNode.h"
#include "OFluxLogging.h"

namespace oflux {
namespace lockfree {
namespace atomic {

Allocator<EventBaseHolder> AtomicCommon::allocator; //(new allocator::MemoryPool<sizeof(EventBaseHolder)>());

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
	while(e && e != wlp->_tail && !is_one(e) && !is_three(e) && e->ev.get()) {
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
	e->next = NULL;
	EventBaseHolder * t = NULL;
	SafeEventBasePtr ev; // hold the event locally
	ev.swap(e->ev);
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
	EventBaseHolder * old_t = NULL;
	while(1) {
		h = _head;
		hn = h->next;
		if(is_one(hn) 
				&& __sync_bool_compare_and_swap(
				  &(_head->next)
				, 0x0001
				, NULL)) {
			// 1->2
			e->ev.swap(ev);
			return true;
		} else {
			// (2,3)->3
			t = _tail;
			old_t = t;
			while(unmk(t) && unmk(t->next)) {
				t = t->next;
			}
			if(t != _tail && t->next == NULL) {
				__sync_bool_compare_and_swap(
					  &_tail
					, old_t
					, t);
			}
			if(unmk(t) && __sync_bool_compare_and_swap(
					  &(t->next)
					, NULL
					, e)) {
				_tail = e;
				t->ev.swap(ev);
				break;
			}
		}
	}
	return false;

}

EventBaseHolder * 
ExclusiveWaiterList::pop()
{
	EventBaseHolder * r = NULL;
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
	EventBaseHolder * t = NULL;
	EventBaseHolder * old_t = NULL;
	while(1) {
		h = _head;
		hn = h->next;
		t = _tail;
		old_t = t;
		while(unmk(t) && unmk(t->next)) {
			t = t->next;
		}
		if(t != _tail && t->next == NULL) {
			__sync_bool_compare_and_swap(
				  &_tail
				, old_t
				, t);
		}
		if(h != _tail && hn != NULL 
				&& !is_one(hn)
				&& h->ev.get() != NULL
				&& __sync_bool_compare_and_swap(
					  &_head
					, h
					, hn)) {
			// 3->2
			r = h;
			r->next = NULL;
			break;
		} else if(hn==NULL && __sync_bool_compare_and_swap(
				  &(_head->next)
				, hn
				, 0x0001)) {
			// 2->1
			break; //empty
		}
	}
	if(r) r->busyWaitOnEv();
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
                at += snprintf(buff+at,5000-at,"%p->",e->ev.get());
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
        oflux_log_trace("  tail = %p\n",_tail->ev.get());
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux

