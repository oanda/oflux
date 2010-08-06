#include "OFluxLFAtomicReadWrite.h"

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


bool
ReadWriteWaiterList::push(EventBaseHolder * e, int type)
{
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
			break;
		} else if( type == EventBaseHolder::Read
			&& rcount == 0
			&& !mkd
			&& !hn
				// 1->3
			&& _head.compareAndSwap(head,h,1,false)
			) {
			res = true;
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
			break;
		} else if( rcount == -1
			&& mkd
			&& hn
				// 4->4
			&& _head.compareAndSwap(head,h,-1,true)
			) {
			add_tail_waiter(e); // macro
			res = false;
			break;
		} else if( type == EventBaseHolder::Read
			&& rcount > 0
			&& !mkd
			&& !hn
				// 3->3
			&& _head.compareAndSwap(head,h,rcount+1,false)
			) {
			res = true;
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
			break;
		} else if( rcount > 0 
			&& mkd
			&& hn
				// 5->5
			&& _head.compareAndSwap(head,h,rcount,true)
			) {
			add_tail_waiter(e); // macro
			res = false;
			break;
		}
	}
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
	&& mkd2 \
	&& _head.compareAndSwap(head2,hn2,rcount2+1,hn2->next!=NULL) \
	&& (h2->next = el) \
	&& (el = h2))

void
ReadWriteWaiterList::pop(EventBaseHolder * & el, int by_type)
{
	EventBaseHolder * h;
	EventBaseHolder * hn;
	int rcount;
	int ht;
	bool mkd;
	el = NULL;
	while(true) {
		RWWaiterHead head(_head);
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
			break;
		} else if( rcount == 1
			&& !mkd
			&& !hn
				// 3->1
			&& _head.compareAndSwap(head,h,0,false)
			) {
			break;
		} else if( rcount > 1
			&& !mkd
			&& !hn
				// 3->3
			&& _head.compareAndSwap(head,h,rcount-1,false)
			) {
			break;
		} else if( rcount == -1
			&& mkd
			&& ht == EventBaseHolder::Write
			&& hn
			&& !hn->next
				// 4->2
			&& _head.compareAndSwap(head,hn,-1,false)
			) {
			el = h;
			el->next = NULL;
			break;
		} else if( rcount == -1
			&& mkd
			&& ht == EventBaseHolder::Write
			&& hn
			&& hn->next
				// 4->4
			&& _head.compareAndSwap(head,hn,-1,true)
			) {
			el = h;
			el->next = NULL;
			break;
		} else if( rcount == -1
			&& mkd
			&& ht == EventBaseHolder::Read
			&& hn
				// 4->(3,5)
			&& _head.compareAndSwap(head,hn,1,hn->next!=NULL)
			) {
			el = h;
			el->next = NULL;
			pop_continue(el);
			break;
		} else if( rcount == 1
			&& mkd
			&& ht == EventBaseHolder::Write
			&& hn
				// 5->(2,4)
			&& _head.compareAndSwap(head,hn,-1,hn->next!=NULL)
			) {
			el = h;
			el->next = NULL;
			break;
		} else if( rcount > 1
			&& mkd
			&& hn
				// 5->5
			&& _head.compareAndSwap(head,h,rcount-1,true)
			) {
			break;
		} else if( rcount == 1
			&& mkd
			&& ht == EventBaseHolder::Read
			&& hn
				// 5->(3,5) [*]
			&& _head.compareAndSwap(head,hn,1,hn->next!=NULL)
			) {
			el = h;
			el->next = NULL;
			pop_continue(el);
			break;
		}
	}
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
