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
	while(1) {
		h = _head;
		hn = h->next;
		t = _tail;
		while(unmk(t) && t->next) {
			t = t->next;
		}
		if(h != t && hn != NULL 
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
	assert(type == EventBaseHolder::Read || type == EventBaseHolder::Write);
	e->next = NULL;
	e->type = EventBaseHolder::None;
	EventBaseHolder * t = NULL;

	EventBasePtr ev;
	ev.swap(e->ev);
	while(1) {
		EventBaseHolder * h = _head;
		EventBaseHolder * hn = h->next;
		EventBase * he = h->ev.get();
		int rc = rcount;
		if(type == EventBaseHolder::Write 
				&& rc == 0
				&& is_one(hn) 
				&& __sync_bool_compare_and_swap(
				  &(_head->next)
				, 0x0001
				, NULL)) {
			// 1->2
			e->ev.swap(ev);
			e->type = type;
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
			e->type = type;
			return true;
		} else if(hn == NULL && rc == 0) {
			if(__sync_bool_compare_and_swap(
					  &(_head->next)
					, NULL
					, e)) {
				// 2->4
				_tail = e;
				h->type = type;
				h->ev.swap(ev);
				return false;
			}
		} else if(is_three(hn)
				&& type == EventBaseHolder::Read
				&& rc > 0
				&& __sync_bool_compare_and_swap(
				  &rcount
				, rc
				, rc+1)) {
			//assert(!is_three(e));
			// 3->3
			e->ev.swap(ev);
			e->type = type;
			return true;
		} else if(is_three(hn)
				&& type == EventBaseHolder::Write) {
			if(__sync_bool_compare_and_swap(
					  &(_head->next)
					, hn
					, e)) {
				// 3->5
				_tail = e;
				h->type = type;
				h->ev.swap(ev);
				return false;
			}
		} else if(!is_three(hn) 
				&& hn != NULL 
				&& !is_one(hn)) {
			// 4->4  or 5->5
			t = _tail;
			while(t && !is_three(t)
					&& !is_one(t)
					&& t->next
					&& !is_three(t->next)
					&& !is_one(t->next)) {
				t = t->next;
			}
			if(h != t && he
					&& h == _head) {
				if(__sync_bool_compare_and_swap(
						  &(t->next)
						, NULL
						, e)) {
					_tail = e;
					t->type = type;
					t->ev.swap(ev);
					break;
				}
			}
		}
	}
	return false;
}



void
ReadWriteWaiterList::pop(EventBaseHolder * & el, int by_type)
{
	EventBaseHolder * h = NULL;
	EventBaseHolder * hn = NULL;
	int rc;
	int orig_rc;
	int ht;
	EventBase * he = NULL;
	el = NULL;


	if(by_type == EventBaseHolder::Read) {
		do {
			orig_rc = rcount;
			rc = orig_rc-1;
			assert(rc >= 0);
		} while( !__sync_bool_compare_and_swap(
			  &rcount
			, orig_rc
			, rc));
	} else {
		orig_rc = rcount;
		rc = orig_rc;
		assert(rc == 0);
	}
	bool is_last_reader = (rc == 0) && (orig_rc == 1);
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
				&& orig_rc > 0
				&& is_last_reader 
				// 3->1
				&& __sync_bool_compare_and_swap(
					  &(_head->next)
					, 0x0003
					, 0x0001)) {
			break;
		} else if(is_three(hn)
				&& orig_rc > 1) {
			// 3->3  (off the hook)
			break;
		} else if(!is_one(hn) 
				&& !is_three(hn) 
				&& hn != NULL
				&& ht == EventBaseHolder::Write
				&& (is_last_reader || orig_rc == 0)
				&& __sync_bool_compare_and_swap(
					  &_head
					, h
					, hn)) {
			// (4,5)->(4,2)
			h->next = NULL;
			el = h;
			break;
		} else if(!is_one(hn) 
				&& !is_three(hn) 
				&& hn != NULL
				&& orig_rc > 0) {
			if(is_last_reader) {
				// 5->(5,3)
				el = NULL;
				int local_reader_count = 0;
				while(ht == EventBaseHolder::Read 
					&& he
					&& !is_one(hn) 
					&& !is_three(hn) 
					&& hn != NULL
					&& __sync_bool_compare_and_swap(
						  &_head
						, h
						, hn)) {
					h->next = el;
					el = h;
					h = _head;
					hn = h->next;
					ht = h->type;
					he = h->ev.get();
					++local_reader_count;
				}
				__sync_fetch_and_add(&rcount,local_reader_count);
				if(el && hn == NULL 
					&& __sync_bool_compare_and_swap(
						  &(_head->next)
						, hn
						, 0x0003)) {}
				if(el) {
					// otherwise nothing happened
					break;
				}
			} else {
				break;
			}
		} else if(!is_one(hn) 
				&& !is_three(hn) 
				&& hn != NULL
				&& ht == EventBaseHolder::Read
				&& rc == 0) {
			// 4->(5,3)
			el = NULL;
			int local_reader_count = 0;
			while(ht == EventBaseHolder::Read 
				&& he
				&& !is_one(hn) 
				&& !is_three(hn) 
				&& hn != NULL
				&& __sync_bool_compare_and_swap(
					  &_head
					, h
					, hn)) {
				h->next = el;
				el = h;
				h = _head;
				hn = h->next;
				ht = h->type;
				he = h->ev.get();
				rc = rcount;
				++local_reader_count;
			}
			__sync_fetch_and_add(&rcount,local_reader_count);
			if(el && hn == NULL 
				&& __sync_bool_compare_and_swap(
					  &(_head->next)
					, hn
					, 0x0003)) {}
			if(el) {
				break;
			}
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
        oflux_log_trace(" atomic[%p] in state %s\n"
                , this
                , state);
	size_t at = 0;
	char buff[5000];
        at += snprintf(buff+at,5000-at,"  head = ");
        EventBaseHolder * e = _head;
        while(e != NULL && !is_one(e) && !is_three(e)) {
                at += snprintf(buff+at,5000-at,"%p%s->"
			, e->ev.get()
			, (e->type == EventBaseHolder::Read ? "R" : (e->type == EventBaseHolder::Write ? "W" : "")));
                e = e->next;
        }
        if(e == NULL) {
                at += snprintf(buff+at,5000-at,"(0x0)\n");
        } else if(is_one(e)) {
                at += snprintf(buff+at,5000-at,"(0x1)\n");
        } else if(is_three(e)) {
                at += snprintf(buff+at,5000-at,"(0x3)\n");
        } else {
                at += snprintf(buff+at,5000-at,"\n");
        }
	oflux_log_trace("%s",buff);
        oflux_log_trace("  tail = %p\n", _tail->ev.get());
        oflux_log_trace("  rcount = %d\n", rcount);
}

void
AtomicReadWrite::log_snapshot_waiters() const
{
	oflux_log_trace(" rcount:%d _wtype:%d\n", _waiters.rcount, _wtype);
	_log_snapshot_waiters(&_waiters); 
}

/////////////////////////////////////////////////////////////////
// PoolEventList state encoding:
//
// 1. (resourcesN) mkd(head) && unmk(head) = { mkd(next) && unmk(next) != NULL, id > 0 }
// 2. (empty) !mkd(head) && head = { next == NULL, id == 0 } 
// 3. (waitingM) !mkd(head) && head = { unmk(next) != NULL, id > 0 }
// Note: in (1) all non-NULL next links are mkd()

PoolEventList::~PoolEventList()
{
	EventBaseHolder * hp = _head.get();
	EventBaseHolder * hp_n = NULL;
	while(unmk(hp)) {
		hp_n = unmk(hp)->next;
		AtomicCommon::allocator.put(unmk(hp));
		hp = hp_n;
	}
}


// push(Event * e) _________________________________________________________
//   Either:
//    return true and *(e->resource_loc) is written with a free resource
//   Or
//    return false and park e in the internal waiters data structure
bool
PoolEventList::push(EventBaseHolder * e)
{
	//*(e->resource_loc) = NULL;
	//e->next = NULL;
	EventBaseHolder * t = NULL;

	StampedPtr<EventBaseHolder> h;
	EventBaseHolder * hp = NULL;
	EventBaseHolder * hn = NULL;
	while(1) {
		h = _head;
		hp = h.get();
		hn = unmk(hp)->next;
		t = _tail;
		while(t->next) {
			t = t->next;
		}
		if(	// condition:
			mkd(hp)
			&& unmk(hn) != NULL) {
			// action:
			*(e->resource_loc) = unmk(hp);
			if(_head.cas(h, hn)) {
				// 1->(1,2)
				return true;
			}
			*(e->resource_loc) = NULL;
		} else if( //condition
			hp
			&& (hp == t)
			&& !mkd(hp)
			&& !hn) {
			// action:
			e->next = hp;
			if(_head.cas(h,e)) {
				// 2->3
				return false;
			}
		} else if( //condition:
			hp 
			&& (hp != t)
			&& !mkd(hp)
			&& !mkd(hn)
			&& hn) {
			// action:

			EventBasePtr ev;
			EventBaseHolder ** resource_loc = e->resource_loc;
			e->next = NULL;
			e->ev.swap(ev);
			resource_loc_asgn(e,NULL);
			e->resource_loc = NULL;

			if(__sync_bool_compare_and_swap(&(t->next),NULL,e)) {
				// 3->3
				_tail = e;
				resource_loc_asgn(t,resource_loc);
				t->resource_loc = resource_loc;
				t->ev.swap(ev);
				return false;
			}
			e->ev.swap(ev);
			resource_loc_asgn(e,resource_loc);
			e->resource_loc = resource_loc;
		}
	}
	return NULL;
}

EventBaseHolder *
PoolEventList::complete_pop()
{
	EventBaseHolder * he = _head.get();
	while(mkd(he) && mkd(unmk(he)->next)) {
		he = unmk(he)->next;
	}
	EventBaseHolder * hen = unmk(he)->next;
	if(hen && !mkd(hen) 
		&& hen->ev.get() != NULL 
		&& hen->resource_loc != NULL
		&& hen->resource == NULL) {
		// proceed
	} else {
		return NULL;
	}
	StampedPtr<EventBaseHolder> h = _head;
	EventBaseHolder * hp = h.get();
	EventBaseHolder * hn = unmk(hp)->next;
	if(mkd(hp) && unmk(hn) && _head.cas(h,hn)) {
		if(__sync_bool_compare_and_swap(&(unmk(he)->next),hen,hen->next)) {
			*(hen->resource_loc) = unmk(hp);
			oflux_log_trace2("  ::complete_pop() success\n");
			return hen;
		} else {
			oflux_log_trace2("  ::complete_pop() fail\n");
			do { 
				h = _head;
			} while(!_head.cas(h,mk(hp)));
		}
	}
	return NULL;
}


// pop(Resource * r) _____________________________________________________
//   Either
//    return re != NULL released event with *(re->release_loc) = r
//   Or
//    return NULL and park resource r in the pool of free resources
EventBaseHolder *
PoolEventList::pop(EventBaseHolder * by_ev)
{
	EventBaseHolder * r = *(by_ev->resource_loc);
	*(by_ev->resource_loc) = NULL;
	StampedPtr<EventBaseHolder> h = NULL;
	EventBaseHolder * hp = NULL;
	EventBaseHolder * hn = NULL;
	EventBaseHolder * t = NULL;
	while(1) {
		h = _head;
		hp = h.get();
		hn = unmk(hp)->next;
		t = _tail;
		while(t->next) {
			t = t->next;
		}
		if(	// condition:
			mkd(hp)
			&& unmk(hn) != NULL
			&& (unmk(hp)->resource != NULL)) {
			// action:
			r->next = mk(hp);
			if(_head.cas(h,mk(r))) {
				// 1->1
				return complete_pop();
			}
		} else if(
			// condition:
			!mkd(hp)
			&& (hp == t)
			&& hp->ev.get() == 0
			&& !hn) {
			// action:
			r->next = hp;
			if(_head.cas(h,mk(r))) {
				// 2->1
				return complete_pop();
			}
		} else if(
			// condition:
			!mkd(hp)
			&& (hp != t)
			&& !mkd(hn)
			&& hp->ev.get() != 0
			&& unmk(hn) != NULL) {
			// action:
			if(_head.cas(h,unmk(hn))) {
				// 3->(2,3)
				*(hp->resource_loc) = r;
				return hp;
			}
		}
	}
	return NULL;
}


void
PoolEventList::dump()
{
	EventBaseHolder * hp = _head.get();
	const char * state =
		( hp == 0 ?
			"undefined  [0]"
		: (mkd(hp) ?
			"resourcesN [1]"
		: (hp->next == NULL ?
			"empty      [2]" :       
			"waitingM   [3]" )));

	oflux_log_trace("[%d]  atomic[%p] in state %s\n"
		, oflux_self()
		, this
		, state);
	char buff[5000];
	size_t at = 0;
	at += snprintf(buff+at,5000-at,"  %s head = "
		, mkd(_head.get()) ? "m" : " ");
	EventBaseHolder * e = _head.get();
	while(unmk(e) != NULL) {
		if(mkd(e)) {
			at += snprintf(buff+at,5000-at,"r%c%p->"
				, mkd(e) ? 'm' : ' ',unmk(e)->resource);
		} else {
			at += snprintf(buff+at,5000-at,"e%c%p->"
				, mkd(e) ? 'm' : ' ',unmk(e)->ev.get());
		}
		if(at >= 5000) return; \
		e  = unmk(e)->next;
	}
	if(e == NULL) {
		at += snprintf(buff+at,5000-at,"(0x0)\n");
	} else if(is_one(e)) {
		at += snprintf(buff+at,5000-at,"(0x1)\n");
	} else {
		at += snprintf(buff+at,5000-at,"\n");
	}
	oflux_log_trace("[%d] %s",oflux_self(),buff);
	oflux_log_trace("[%d]   tail = e%p/r%p\n"
		, oflux_self()
		, _tail->ev.get()
		, _tail->resource);
}

// clarify when _resource_ebh is what:
//------------------------------------
//
// constructed:  emptied ebh (has resource set to NULL)
// acquired:   populated ebh (resource set to not NULL)
// released:
//


AtomicPooled::AtomicPooled(AtomicPool * pool,void * data)
	: _next(NULL)
	, _pool(pool)
	, _resource_ebh(AtomicCommon::allocator.get(data))
	, _by_ebh(AtomicCommon::allocator.get('\0',&_resource_ebh))
{
	oflux_log_trace2("AC::alloc.g APD*this:%p _resource_ebh:%p %d\n"
		, this
		, _resource_ebh
		, __LINE__);
	oflux_log_trace2("AC::alloc.g APD*this:%p _by_ebh:%p %d\n"
		, this
		, _by_ebh
		, __LINE__);
	init();
	oflux_log_trace2("APD::APD APD*this:%p\n",this);
	//check();
}

void
AtomicPooled::relinquish()
{
	oflux_log_trace2("[%d] APD::relinquish APD*this:%p\n"
		, oflux_self()
		, this);
	_pool->release(this);
	//check();
}

const char *
AtomicPooled::atomic_class_str = "lockfree::Pooled";

bool
AtomicPooled::acquire_or_wait(EventBasePtr & ev,int t)
{
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	assert(_by_ebh);
	oflux_log_trace("[%d] AP::a_o_w ev %s ev*:%p  atom APD*this:%p "
		" _by_ebh:%p _by_ebh->rsrc_loc: %p\n"
		, oflux_self()
		, ev->flow_node()->getName()
		, ev.get()
		, this
		, _by_ebh
		, _by_ebh->resource_loc);
	_by_ebh->ev = ev;
	_by_ebh->type = t;
	if(_resource_ebh) {
		assert(_resource_ebh->resource == NULL);
		oflux_log_trace2("AC::alloc.p _resource_ebh:%p %d\n"
			, _resource_ebh
			, __LINE__);
		AtomicCommon::allocator.put(_resource_ebh);
		_resource_ebh = NULL;
	}
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	EventBaseHolder *local_by_ebh = _by_ebh;
	_by_ebh = AtomicCommon::allocator.get('\0',&_resource_ebh);
	oflux_log_trace2("AC::alloc.g %p %d\n",_by_ebh,__LINE__);
	init();
	bool acqed = _pool->waiters.push(local_by_ebh); // try to acquire the resource
	assert(!acqed || _resource_ebh->resource);
	if(acqed) {
		AtomicCommon::allocator.put(local_by_ebh);
	}
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	oflux_log_trace("[%d] AP::a_o_w ev %s ev*:%p APD*this:%p res %d\n"
		, oflux_self()
		, ev->flow_node()->getName()
		, ev.get()
		, this
		, acqed);
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	return acqed;
}

void
AtomicPooled::release(
	  std::vector<EventBasePtr> & rel_ev_vec
	, EventBasePtr &ev)
{
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	assert(_resource_ebh);
	assert(_resource_ebh->resource != NULL);
	oflux_log_trace2("[%d] AP::rel   ev %s ev*:%p  APD*this:%p _by_ebh:%p _res_ebh->res:%p\n"
		, oflux_self()
		, ev.get() ? ev->flow_node()->getName() : "<NULL>"
		, ev.get()
		, this
		, _by_ebh
		, _resource_ebh->resource);
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	EventBaseHolder * rel_ebh = _pool->waiters.pop(_by_ebh);
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	if(rel_ebh) { 
		assert(rel_ebh != _by_ebh);
		assert(rel_ebh->ev.get());
		assert((*(rel_ebh->resource_loc))->resource);
		oflux_log_trace("[%d] AP::rel   ev %s ev*:%p APD*this:%p passed to %s rel_ebh->ev:%p rel_ebh->res_loc:%p rsrc:%p\n"
			, oflux_self()
			, ev.get() ? ev->flow_node()->getName() : "<NULL>"
			, ev.get()
			, this
			, rel_ebh->ev->flow_node()->getName()
			, rel_ebh->ev.get()
			, rel_ebh->resource_loc
			, (*(rel_ebh->resource_loc))->resource);
		//_by_ebh.ev = rel_ebh->ev;
		//_by_ebh.type = rel_ebh->type;
		rel_ev_vec.push_back(rel_ebh->ev);
		oflux_log_trace2("AC::alloc.p rel_ebh:%p %d\n"
			, rel_ebh
			, __LINE__);
		AtomicCommon::allocator.put(rel_ebh);
	} else {
		oflux_log_trace("[%d] AP::rel   ev %s ev*:%p  APD*this:%p rsrc nothing out\n"
			, oflux_self()
			, ev.get() ? ev->flow_node()->getName() : "<NULL>"
			, ev.get()
			, this);
	}
	static int * _null = NULL;
	_resource_ebh = AtomicCommon::allocator.get(_null);
	oflux_log_trace2("AC::alloc.g _res_ebh:%p %d\n",_resource_ebh,__LINE__);
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	if(!rel_ebh) {
		_pool->release(this);
	}
}

Allocator<AtomicPooled> AtomicPool::allocator; // (new allocator::MemoryPool<sizeof(AtomicPooled)>());

AtomicPool::AtomicPool()
	: head_free(NULL)
{
}

AtomicPool::~AtomicPool()
{
	oflux_log_trace2("~AtomicPool APD*this:%p\n",this);
	// reclaim items from the free list
	AtomicPooled * aph = head_free;
	AtomicPooled * aph_n = NULL;
	while(aph) {
		aph_n = aph->_next;
		AtomicPool::allocator.put(aph);
		aph = aph_n;
	}
}

void
AtomicPool::release(AtomicPooled *ap)
{
	oflux_log_trace2("[%d] AP::release APD*:%p\n",oflux_self(),ap);
	ap->init();
	// put it on the free list
	AtomicPooled * h;
	do {
		h = head_free;
		ap->_next = h;
	} while(!__sync_bool_compare_and_swap(&head_free,h,ap));
}

const void *
AtomicPool::get(oflux::atomic::Atomic * & a_out,const void *)
{
	char _here;
	static const char * p_here = &_here;
	AtomicPooled * ap_out = NULL;
	while((ap_out = head_free) 
			&& !__sync_bool_compare_and_swap(
				  &head_free
				, ap_out
				, ap_out->_next));
	if(!ap_out) {
		void * p = NULL;
		ap_out = AtomicPool::allocator.get(this,p);
		oflux_log_trace2("[%d] AP::get APD*:%p new\n",oflux_self(),ap_out);
	} else {
		oflux_log_trace2("[%d] AP::get APD*:%p old\n",oflux_self(),ap_out);
	}
	a_out = ap_out;
	//ap_out->check();
	return &p_here;
}

void
AtomicPooled::check()
{
	assert(_by_ebh->resource_loc == &_resource_ebh);
}

class AtomicPoolWalker : public oflux::atomic::AtomicMapWalker {
public:
	AtomicPoolWalker(PoolEventList &pool);
	virtual ~AtomicPoolWalker();
	virtual bool next(const void * & key,oflux::atomic::Atomic * &atom);
private:
};

AtomicPoolWalker::AtomicPoolWalker(PoolEventList & pool)
{
}

AtomicPoolWalker::~AtomicPoolWalker()
{
}

bool 
AtomicPoolWalker::next(const void * &, oflux::atomic::Atomic * &atom)
{
	bool res = false;
	return res;
}

oflux::atomic::AtomicMapWalker *
AtomicPool::walker()
{
	return new AtomicPoolWalker(waiters);
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux

