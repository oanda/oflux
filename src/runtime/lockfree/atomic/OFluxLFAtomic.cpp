#include "lockfree/atomic/OFluxLFAtomic.h"
#include "lockfree/allocator/OFluxLFMemoryPool.h"
#include "flow/OFluxFlowNode.h"
#include "OFluxLogging.h"

namespace oflux {
namespace lockfree {
namespace atomic {

#define is_one    is_val<0x0001>
#define is_three  is_val<0x0003>
#define set_one   set_val<0x0001>
#define set_three set_val<0x0003>

Allocator<EventBaseHolder> AtomicCommon::allocator; //(new allocator::MemoryPool<sizeof(EventBaseHolder)>());

static const char *
convert_wtype_to_string(int wtype)
{
	static const char * conv[] =
		{ "None "
		, "Read "
		, "Write"
		, "Excl."
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

bool
ExclusiveWaiterList::push(EventBaseHolder *e)
{
	e->next = NULL;
	EventBaseHolder * t = NULL;
	EventBasePtr ev; // hold the event locally
	ev.swap(e->ev);
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
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
			// 2->3
			t = _tail;
			while(unmk(t) && t->next) {
				t = t->next;
			}
			if(unmk(t) && __sync_bool_compare_and_swap(&(t->next),NULL,e)) {
				t->ev.swap(ev);
				_tail = e;
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
	while(1) {
		h = _head;
		hn = h->next;
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

        printf(" atomic[%p] in state %s\n"
                , this
                , state);
        printf("  head = ");
        EventBaseHolder * e = _head;
        while(e != NULL && !is_one(e)) {
                printf("%p->",e->ev.get());
                e  = e->next;
        }
        if(e == NULL) {
                printf("(0x0)\n");
        } else if(is_one(e)) {
                printf("(0x1)\n");
        } else {
                printf("\n");
        }
        printf("  tail = %p\n",_tail->ev.get());
}


///////////////////////////////////////////////////////////////////////////////
// ReadWriteWaiterList state encoding:
// 
// 1. (empty      : 0 readers, 0 writer, 0 waiters) 
// 	elist { head == tail && head->next = 0x1 }
// 	rcount == 0
// 2. (exclusive0 : 0 readers, 1 writer, 0 waiters)
// 	elist { head == tail && head->next = 0x0 }
// 	rcount == 0
// 3. (readingN0  : N readers, 0 writer, 0 waiters)
// 	elist { head == tail && head->next = 0x3 }
// 	rcount > 0
// 4. (exclusiveM : 0 readers, 1 writer, M waiters)
// 	elist { head != tail && head->next !in {0x0,0x1,0x3} }
// 	rcount == 0
// 5. (readingNM  : N readers, 0 writer, M waiters)
// 	elist { head != tail && head->next !in {0x0,0x1,0x3 } }
// 	rcount > 0
// 
// predicates:
//  in_R() = head->next == 0x3 || rcount > 0
//  in_W() = (head->next == 0x0  || head->next > 0x3)
//              && rcount == 0


bool
ReadWriteWaiterList::push(
	  EventBaseHolder * e
	, int type)
{
	e->next = NULL;
	EventBaseHolder * t = NULL;

	EventBasePtr ev;
	ev.swap(e->ev);
	while(1) {
		EventBaseHolder * h = _head;
		EventBaseHolder * hn = h->next;
		EventBase * he = h->ev.get();
		int rc = rcount;
		if(type == EventBaseHolder::Write 
				&& is_one(hn) 
				&& __sync_bool_compare_and_swap(
				  &(_head->next)
				, 0x0001
				, NULL)) {
			// 1->2
			e->ev.swap(ev);
			return true;
		} else if(type == EventBaseHolder::Read
				&& rc == 0
				&& is_one(hn)
				&& __sync_bool_compare_and_swap(
				  &(_head->next)
				, 0x0001
				, 0x0003)) {
			// 1->3
			__sync_bool_compare_and_swap(
				  &rcount
				, rc
				, 1);
			e->ev.swap(ev);
			return true;
		} else if(hn == NULL
				&& __sync_bool_compare_and_swap(
				  &(_head->next)
				, NULL
				, e)) {
			// 2->4
			h->ev.swap(ev);
			h->type = e->type;
			e->type = EventBaseHolder::None;
			return false;
		} else if(is_three(hn)
				&& type == EventBaseHolder::Read
				&& rc > 0
				&& __sync_bool_compare_and_swap(
				  &rcount
				, rc
				, rc+1)) {
			assert(!is_three(e));
			// 3->3
			e->ev.swap(ev);
			return true;
		} else if(is_three(hn)
				&& type == EventBaseHolder::Write
				&& __sync_bool_compare_and_swap(
				  &(_head->next)
				, hn
				, e)) {
			// 3->5
			h->ev.swap(ev);
			h->type = e->type;
			e->type = EventBaseHolder::None;
			return false;
		} else if(!is_three(hn) 
				&& hn != NULL 
				&& !is_one(hn)) {
			// 4->4  or 5->5
			while(_tail->next) {
				_tail = _tail->next;
			}
			t = _tail;
			if(_head != _tail && he
					&& __sync_bool_compare_and_swap(
						 &(t->next)
						,NULL
						,e)) {
				t->ev.swap(ev);
				t->type = e->type;
				e->type = EventBaseHolder::None;
				_tail = e;
				break;
			}
		}
	}
	return false;
}



void
ReadWriteWaiterList::pop(EventBaseHolder * & el, EventBasePtr & by_e)
{
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
	int rc = rcount;
	int orig_rc = rc;
	int ht;
	EventBase * he = NULL;
	el = NULL;
	bool is_last_reader = 
		( rc > 0
		? __sync_fetch_and_sub(&rcount,1) == 1
		: true);
	while(1) {
		h = _head;
		hn = h->next;
		ht = h->type;
		he = h->ev.get();
		rc = rcount;
		if(hn == NULL
				&& orig_rc == 0
				&& __sync_bool_compare_and_swap(
					  &(_head->next)
					, hn
					, 0x0001)) {
			// 2->1
			break;
		} else if(is_three(hn)
				&& orig_rc > 0) {
			if( is_last_reader ) {
				// 3->1
				__sync_bool_compare_and_swap(&(_head->next),0x0003,0x0001);
			}
			break;
		} else if(!is_one(hn) && !is_three(hn) && hn != NULL
				&& ht == EventBaseHolder::Write
				&& (is_last_reader || orig_rc == 0)
				&& __sync_bool_compare_and_swap(
					  &_head
					, h
					, hn)) {
			// (4,5)->(4,2)
			if(hn->ev.get() == 0) {
				_tail = _head;
			}
			h->next = NULL;
			el = h;
			break;
		} else if(!is_one(hn) && !is_three(hn) && hn != NULL
				&& orig_rc > 0) {
			if(is_last_reader) {
				// 5->(5,3)
				el = NULL;
				while(ht == EventBaseHolder::Read && he
					&& !is_one(hn) && !is_three(hn) && hn != NULL
					&& __sync_bool_compare_and_swap(
						  &_head
						, h
						, hn)) {
					__sync_bool_compare_and_swap(
						  &rcount
						, rc
						, std::min(0,rc-1));
					h->next = el;
					el = h;
					h = _head;
					hn = h->next;
					ht = h->type;
					he = h->ev.get();
					rc = rcount;
				}
			}
			break;
		} else if(!is_one(hn) && !is_three(hn) && hn != NULL
				&& ht == EventBaseHolder::Read
				&& orig_rc == 0) {
			// 4->(5,3)
			el = NULL;
			while(ht == EventBaseHolder::Read && he
				&& !is_one(hn) && !is_three(hn) && hn != NULL
				&& __sync_bool_compare_and_swap(
					  &_head
					, h
					, hn)) {
				if(hn->ev.get() == NULL) {
					_tail = hn;
				}
				h->next = el;
				el = h;
				h = _head;
				__sync_fetch_and_add(&rcount,1);
				hn = h->next;
				ht = h->type;
				he = h->ev.get();
				rc = rcount;
			}
			if(hn == NULL && __sync_bool_compare_and_swap(&(_head->next),hn,0x0003)) {}
			break;
		}
	}
}


void
ReadWriteWaiterList::dump()
{
        const char * state =
                (_head->next == NULL ?
                        "exclusive0 [2]"
                : (is_one(_head->next) ?
                        "empty      [1]"
                : (is_three(_head->next) ?
                        "readingN0  [3]"
                : (!rcount ?
                        "exclusiveM [4]" :
                        "readingNM  [5]"))));
        printf(" atomic[%p] in state %s\n"
                , this
                , state);
        printf("  head = ");
        EventBaseHolder * e = _head;
        while(e != NULL && !is_one(e) && !is_three(e)) {
                printf("%p%s->"
			, e->ev.get()
			, (e->type == EventBaseHolder::Read ? "R" : (e->type == EventBaseHolder::Write ? "W" : "")));
                e = e->next;
        }
        if(e == NULL) {
                printf("(0x0)\n");
        } else if(is_one(e)) {
                printf("(0x1)\n");
        } else if(is_three(e)) {
                printf("(0x3)\n");
        } else {
                printf("\n");
        }
        printf("  tail = %p\n", _tail->ev.get());
        printf("  rcount = %d\n", rcount);
}

void
AtomicReadWrite::log_snapshot_waiters() const
{
	oflux_log_trace(" rcount:%d _wtype:%d\n", _waiters.rcount, _wtype);
	_log_snapshot_waiters(&_waiters); 
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux

