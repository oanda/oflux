#include "OFluxLFAtomicReadWrite.h"
#include <cstdio>

namespace oflux {
namespace lockfree {
namespace atomic {

/////////////////////////////////////////////////////////////////////////////
// ReadWriteWaiterList States:
//
// 1. empty
// 	{ _head = { ev=0; type=0; next=0; }; rcount = 0; mkd = false;}
// 	_tail = _head;
// 2. exclusive0
// 	{ _head = { ev=0; type=0; next=0; }; rcount = -1; mkd = false;}
// 	_tail = _head;
// 3. readingN0
// 	{ _head = { ev=0; type=0; next=0; }; rcount > 0; mkd=false;}
// 	_tail = _head;
// 4. exclusiveM
// 	{ _head = { ev!=0; type!=0; next!=0; }; rcount = -1; mkd=true;}
// 	_tail = { ev=0; type=0; next=0; }
//            != _head
// 5. readingNM
// 	{ _head = { ev!=0; type!=0; next!=0; }; rcount > 0; mkd=true;}
// 	_tail = { ev=0; type=0; next=0; }
//            != _head

#define INVARIANT \
  { RWWaiterHead head_check(_head); \
    EventBaseHolder * h_check = head_check.head(); \
    int rcount_check = head_check.rcount(); \
    bool mkd_check = head_check.mkd() || h_check->next; \
    assert( (rcount_check >= -1) || (_head.u64() != head_check.u64()) ); \
    assert( ((mkd_check) ^ (h_check->next == NULL)) || (_head.u64() != head_check.u64())); \
    assert( ((h_check->next == NULL) || (rcount_check != 0)) || (_head.u64() != head_check.u64())); \
  }
    

#define add_tail_waiter(e) \
  EventBaseHolder * t; \
  e->next = NULL; \
  do { \
    t = _tail; \
    while(t && t->next) { \
      t = t->next; \
    } \
  } while(!__sync_bool_compare_and_swap(&(t->next),NULL,e)); \
  _tail = e; \
  t->type = type; \
  t->ev.swap(ev);

ReadWriteWaiterList::~ReadWriteWaiterList()
{
	EventBaseHolder * e = _head.head();
	EventBaseHolder * en;
	while((en = e->next)) {
		AtomicCommon::allocator.put(e);
		e = en;
	}
	if(e) {
		AtomicCommon::allocator.put(e);
	}
}

bool
ReadWriteWaiterList::push(EventBaseHolder * e, int type)
{
	Obs & obs = log.submit();
	obs.act = ReadWriteWaiterList::Obs::act_Push;
	obs.term_index = 0;
	obs.e=e;
	obs.ev=e->ev.get();
	obs.type=type;
	obs.res=false;
	obs.term_index= 0;
	obs.trans = "";
	obs.tid = oflux_self();
	bool res = false;
	EventBasePtr ev;
	ev.swap(e->ev);
	EventBaseHolder * hn;
	EventBaseHolder * h;
	int rcount;
	bool mkd;
	e->type = EventBaseHolder::None;
	while(true) {
                RWWaiterHead head(_head);
		obs.u64 = head.u64();
		h = head.head();
		hn = h->next;
		rcount = head.rcount();
		mkd = head.mkd() || hn;
		if(        type == EventBaseHolder::Write
			&& rcount == 0 
			&& !mkd 
			&& !hn
				// 1->2
			&& _head.compareAndSwap(head,h,-1,false)
			) {
			res = true;
			obs.trans = "1->2";
			break;
		} else if( type == EventBaseHolder::Read
			&& rcount == 0
			&& !mkd
			&& !hn
				// 1->3
			&& _head.compareAndSwap(head,h,1,false)
			) {
			res = true;
			obs.trans = "1->3";
			break;
		} else if( rcount == -1
			&& !mkd
			&& !hn
				// 2->4
			&& (e->next = h)
			&& _head.compareAndSwap(head,e,-1,true)
			) {
			res = false;
			e->type = type;
			e->ev.swap(ev);
			obs.trans = "2->4";
			break;
		} else if( rcount == -1
			&& mkd
			&& hn
				// 4->4
			&& _head.compareAndSwap(head,h,-1,true)
			) {
			add_tail_waiter(e); // macro
			res = false;
			obs.trans = "4->4";
			break;
		} else if( type == EventBaseHolder::Read
			&& rcount > 0
			&& !mkd
			&& !hn
				// 3->3
			&& _head.compareAndSwap(head,h,rcount+1,false)
			) {
			res = true;
			obs.trans = "3->3";
			break;
		} else if( type == EventBaseHolder::Write
			&& rcount > 0
			&& !mkd
			&& !hn
				// 3->5
			&& (e->next = h)
			&& _head.compareAndSwap(head,e,rcount,true)
			) {
			res = false;
			e->type = type;
			e->ev.swap(ev);
			obs.trans = "3->5";
			break;
		} else if( rcount > 0 
			&& mkd
			&& hn
				// 5->5
			&& _head.compareAndSwap(head,h,rcount,true)
			) {
			add_tail_waiter(e); // macro
			res = false;
			obs.trans = "5->5";
			break;
		}
	}
	obs.term_index = log.at();
	obs.res = res;
	obs.act = ReadWriteWaiterList::Obs::act_push;
	INVARIANT
	return res;
}

#define pop_continue(el) \
  EventBaseHolder * h2; \
  EventBaseHolder * hn2; \
  int rcount2; \
  bool mkd2; \
  RWWaiterHead head2; \
  do { \
    head2 = _head; \
    h2 = head2.head(); \
    rcount2 = head2.rcount(); \
    hn2 = h2->next; \
    mkd2 = head2.mkd(); \
  } while( h2->type == EventBaseHolder::Read \
	&& h2->next != NULL \
	&& h2->ev.get() \
	&& mkd2 \
	&& _head.compareAndSwap(head2,hn2,rcount2+1,hn2->next!=NULL) \
	&& (h2->next = el) \
	&& (el = h2))

void
ReadWriteWaiterList::pop(EventBaseHolder * & el, int by_type)
{
	Obs & obs = log.submit();
	obs.act=ReadWriteWaiterList::Obs::act_Pop;
	obs.e=NULL;
	obs.ev=NULL;
	obs.trans = "";
	obs.type = by_type;
	obs.res=false;
	obs.tid = oflux_self();
	obs.term_index = 0;
	EventBaseHolder * h;
	EventBaseHolder * hn;
	int rcount;
	int ht;
	bool mkd;
	el = NULL;
	while(true) {
		RWWaiterHead head(_head);
		obs.u64 = head.u64();
		h = head.head();
		hn = h->next;
		rcount = head.rcount();
		ht = h->type;
		mkd = head.mkd() || hn;
		if(        rcount == -1
			&& !mkd
			&& !hn
				// 2->1
			&& _head.compareAndSwap(head,h,0,false)
			) {
			obs.trans = "2->1";
			break;
		} else if( rcount == 1
			&& !mkd
			&& !hn
				// 3->1
			&& _head.compareAndSwap(head,h,0,false)
			) {
			obs.trans = "3->1";
			break;
		} else if( rcount > 1
			&& !mkd
			&& !hn
				// 3->3
			&& _head.compareAndSwap(head,h,rcount-1,false)
			) {
			obs.trans = "3->3";
			break;
		} else if( rcount == -1
			&& mkd
			&& ht == EventBaseHolder::Write
			&& h->ev.get()
			&& hn
			&& !hn->next
				// 4->2
			&& _head.compareAndSwap(head,hn,-1,false)
			) {
			el = h;
			el->next = NULL;
			obs.trans = "4->2";
			break;
		} else if( rcount == -1
			&& mkd
			&& ht == EventBaseHolder::Write
			&& h->ev.get()
			&& hn
			&& hn->next
				// 4->4
			&& _head.compareAndSwap(head,hn,-1,true)
			) {
			el = h;
			el->next = NULL;
			obs.trans = "4->4";
			break;
		} else if( rcount == -1
			&& mkd
			&& ht == EventBaseHolder::Read
			&& h->ev.get()
			&& hn
				// 4->(3,5)
			&& _head.compareAndSwap(head,hn,1,hn->next!=NULL)
			) {
			el = h;
			el->next = NULL;
			pop_continue(el);
			obs.trans = "4->(3,5)";
			break;
		} else if( rcount == 1
			&& mkd
			&& h->ev.get()
			&& ht == EventBaseHolder::Write
			&& hn
				// 5->(2,4)
			&& _head.compareAndSwap(head,hn,-1,hn->next!=NULL)
			) {
			el = h;
			el->next = NULL;
			obs.trans = "5->(2,4)";
			break;
		} else if( rcount > 1
			&& mkd
			&& hn
				// 5->5
			&& _head.compareAndSwap(head,h,rcount-1,true)
			) {
			obs.trans = "5->5";
			break;
		} else if( rcount == 1
			&& mkd
			&& h->ev.get()
			&& ht == EventBaseHolder::Read
			&& hn
				// 5->(3,5) [*]
			&& _head.compareAndSwap(head,hn,1,hn->next!=NULL)
			) {
			el = h;
			el->next = NULL;
			pop_continue(el);
			obs.trans = "5->(3,5) [*]";
			break;
		}
	}
	if(el) el->busyWaitOnEv();
	obs.e = el;
	obs.ev = (el ? el->ev.get() : NULL);
	obs.act=ReadWriteWaiterList::Obs::act_pop;
	obs.term_index = log.at();
	INVARIANT
}

void
ReadWriteWaiterList::dump()
{
	RWWaiterHead head(_head);
	EventBaseHolder * tail = _tail;
	const char * state =
		(head.rcount() == 0 ?
                        "empty      [1]"
		: (head.rcount() > 0 ?
		  (head.mkd() ?
                        "readingNM  [5]"
		  :
                        "readingN0  [3]"
		  )
		: (head.mkd() ?
                        "exclusiveM [4]"
		  :
                        "exclusive0 [2]"
		  )));
        oflux_log_trace(" atomic[%p] in state %s\n"
                , this
                , state);
	size_t at = 0;
	char buff[5000];
        at += snprintf(buff+at,5000-at,"  head = ");
        EventBaseHolder * e = head.head();
        while(e != NULL) {
                at += snprintf(buff+at,5000-at,"%p%s->"
			, e->ev.get()
			, (e->type == EventBaseHolder::Read ? "R" : (e->type == EventBaseHolder::Write ? "W" : "")));
                e = e->next;
        }
        if(e == NULL) {
                at += snprintf(buff+at,5000-at,"(0x0)\n");
        } else {
                at += snprintf(buff+at,5000-at,"\n");
        }
	oflux_log_trace("%s",buff);
        oflux_log_trace("  tail = %p\n", tail->ev.get());
        oflux_log_trace("  rcount = %d\n", head.rcount());
        oflux_log_trace("  mkd = %d\n", head.mkd());
}

size_t
ReadWriteWaiterList::count() const
{
	EventBaseHolder * e = _head.head();
	size_t res = 0;
	while(e && e->next) {
		e = e->next;
		++res;
	}
	return res;
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux
