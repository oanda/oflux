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
//
// state:
//  resourcesN [1]
//   mkd(head)
//   && mkd(unmk(head)->next)
//   && unmk(unmk(head)->next) != NULL
// trans:
//  push.1->(1,2)
//  pop.1->1
//
// state:
//  empty [2]
//   !mkd(head)
//   && head->next == NULL
//   && head->con.id == 0
// trans:
//  push.2->3
//  push.3->3  (really the same as 2->3)
//  pop.2->1
//  pop.3->(2,3) 
//          h->con.id == 0 on the sentinal
//          unmk(hn) != NULL
//
// state:
//  waitingM [3]
//   !mkd(head)
//   && !mkd(head->next)
//   && head->next != NULL
//   && head->con.id > 0
// trans:
//  push.3->3
//  push.2->3  (really the same as 3->3)
//  pop.3->(2,3)
//  pop.2->1
//            h->con.id == 0
//
//
//
//
//
//
//


#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <deque>
#include <cassert>
#include <stdint.h>

size_t num_items = 5;
int * items = NULL;

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

namespace event {
struct Event {
	Event(int i, Event ** rl) 
		: has(-1)
		, waiting_on(-1)
		, resource_loc(rl)
		, next(NULL) 
	{ con.id = i; }

	Event(int * it) 
		: has(-1)
		, waiting_on(-1)
		, resource_loc(NULL)
		, next(NULL)
	{ con.item = it; }

	int has;
	int waiting_on;
	union Con { 
		int id;     // id of event
		int * item; // ptr to resource for storing Resources
	} con;
	Event ** resource_loc; // location to write the resource ptr
	Event * next;
};

typedef Event Resource;

/*
struct LocalEventList { // use in a single thread
	LocalEventList() : head(NULL), tail(NULL) {}

	void push(Event *e) {
		e->next = NULL;
		if(tail) {
			tail->next = e;
			tail = e;
		} else {
			head = e;
			tail = e;
		}
	}
	Event * pop() {
		Event * r = NULL;
		if(head) {
			r = head;
			if(tail == head) {
				tail = NULL;
			}
			head = head->next;
			r->next = NULL;
		}
		return r;
	}
		

	Event * head;
	Event * tail;
};
*/

template<typename T>
inline T * mk(T * p)
{
	size_t u = reinterpret_cast<size_t>(p);
	return reinterpret_cast<T *>(u | 0x0001);
}

template<typename T>
inline T * unmk(T * p)
{
	size_t u = reinterpret_cast<size_t>(p);
	return reinterpret_cast<T *>(u & ~0x0001);
}

template<typename T>
inline bool mkd(T * p)
{
	return reinterpret_cast<size_t>(p) & 0x0001;
}

template<typename T>
inline bool is_one(T* p)
{
	return reinterpret_cast<size_t>(p) == 0x0001;
}

struct PoolEventList { // thread safe
	PoolEventList() : head(new Event(0,0)), tail(head.get())
	{}
	bool push(Event *e); // acquire a pool item

	Event * pop(Resource * r); //release a pool item

	StampedPtr<Event> head;
	Event * tail;
	// state encoding:
	//
	// 1. (resourcesN) mkd(head) && unmk(head) = { mkd(next) && unmk(next) != NULL, id > 0 }
	// 2. (empty) !mkd(head) && head = { next == NULL, id == 0 } 
	// 3. (waitingM) !mkd(head) && head = { unmk(next) != NULL, id > 0 }
	// Note: in (1) all non-NULL next links are mkd()
};


// push(Event * e) _________________________________________________________
//   Either:
//    return true and *(e->resource_loc) is written with a free resource
//   Or
//    return false and park e in the internal waiters data structure
bool
PoolEventList::push(Event * e)
{
	//*(e->resource_loc) = NULL;
	//e->next = NULL;
	//Event * t = NULL;

	//int id = e->con.id;
	//int w_on = e->waiting_on;
	//int hs = e->has;
	//e->con.id = 0;
	//e->waiting_on = -1;
	//e->has = -1;
	StampedPtr<Event> h;
	Event * hp = NULL;
	Event * hn = NULL;
	while(1) {
		h = head;
		hp = h.get();
		hn = unmk(hp)->next;
		/* not used currently:
		t = tail;
		while(t->next) {
			t = t->next;
		}
		*/
		if(	// condition:
			mkd(hp)
			&& mkd(hn)
			&& unmk(hn) != NULL) {
			// action:
			*(e->resource_loc) = unmk(hp);
			if(head.cas(h, unmk(hn)->con.item ? hn : unmk(hn))
				) {
				// 1->(1,2)
				//unmk(h)->next = NULL;
				//e->next = NULL;
				return true;
			}
			*(e->resource_loc) = NULL;
		} else if( //condition
			hp
			&& (hp == tail)
			&& !mkd(hp)
			&& !hn) {
			// action:
			e->next = hp;
			if(head.cas(h,e)) {
				printf("  ::pushed(1) head from %d to %d\n"
					, hp->con.id
					, e->con.id);
				// 2->3
				return false;
			}
			//e->next = NULL;
		} else if( //condition:
			hp 
			&& (hp != tail)
			&& !mkd(hp)
			&& !mkd(hn)
			&& hn) {
			// action:
			e->next = hp;
			if(head.cas(h,e)) {
				// 3->3
				// parking it on the head (not starvation free!)
				// TODO: stop starving
				printf("  ::pushed(1) head from %d to %d\n"
					, hp->con.id
					, e->con.id);
				return false;
			}
			//e->next = NULL;
		}
	}
	return NULL;
}

// pop(Resource * r) _____________________________________________________
//   Either
//    return re != NULL released event with *(re->release_loc) = r
//   Or
//    return NULL and park resource r in the pool of free resources
Event *
PoolEventList::pop(Event * by_ev)
{
	Resource * r = *(by_ev->resource_loc);
	*(by_ev->resource_loc) = NULL;
	StampedPtr<Event> h = NULL;
	Event * hp = NULL;
	Event * hn = NULL;
	while(1) {
		h = head;
		hp = h.get();
		hn = unmk(hp)->next;
		if(	// condition:
			mkd(hp)
			&& mkd(hn)
			&& unmk(hn) != NULL
			&& (hp->con.item != NULL)) {
			// action:
			r->next = mk(hp);
			if(head.cas(h,mk(r))) {
				// 1->1
				return NULL;
			}
			//r->next = NULL;
		} else if(
			// condition:
			!mkd(hp)
			&& (hp == tail)
			&& hp->con.id == 0
			&& !hn) {
			// action:
			r->next = mk(hp);
			if(head.cas(h,mk(r))) {
				// 2->1
				// if putting events on the tail
				// will need to see if 
				// unmk(r->next)->con.id > 0
				// -- if so pull it out
				// cas-ing r->next
				return NULL;
			}
			//r->next = NULL;
		} else if(
			// condition:
			!mkd(hp)
			&& (hp != tail)
			&& !mkd(hn)
			&& hp->con.id > 0
			&& unmk(hn) != NULL) {
			// action:
			if(head.cas(h,unmk(hn))) {
				// 3->(2,3)
				assert(hp->con.id);
				//h->next = NULL;
				*(hp->resource_loc) = r;
				//r->next = NULL;
				printf("  ::pop head from %d to %d\n"
					, hp->con.id
					, hn->con.id);
				return hp;
			}
		}
	}
	return NULL;
}

} // namespace event

namespace atomic {

struct Pool {
	static event::PoolEventList default_pel;

	Pool(event::PoolEventList & pel = default_pel)
		: waiters(pel) // shared
		, resource(NULL)
	{}
	void release(event::Event * & rel_ev, event::Event * by_ev);
	bool acquire_or_wait(event::Event * e);

	int * data()
	{ return (  resource ? resource->con.item : NULL); }

	void dump(int);

	event::PoolEventList & waiters;

	event::Resource * resource;
	int index;
};

event::PoolEventList Pool::default_pel;


void
Pool::dump(int ip)
{
	const char * state =
		( waiters.head.get() == 0 ?
			"undefined  [0]"
		: (event::mkd(waiters.head.get()) ?
			"resourcesN [1]"
		: (waiters.head.get()->next == NULL ?
			"empty      [2]" :       
			"waitingM   [3]" )));

	printf("%d] atomic[%d] in state %s\n"
		, ip
		, index
		, state);
	char buff[5000];
	size_t at = 0;
	at += snprintf(buff+at,5000-at,"%d] %s head = "
		, ip
		, mkd(waiters.head.get()) ? "m" : " ");
	event::Event * e = waiters.head.get();
	while(unmk(e) != NULL) {
		if(!mkd(e)) {
			at += snprintf(buff+at,5000-at,"%c%d->"
				, mkd(e) ? 'm' : ' ',unmk(e)->con.id);
		} else {
			at += snprintf(buff+at,5000-at,"%c%p->"
				, mkd(e) ? 'm' : ' ',unmk(e)->con.item);
		}
		if(at >= 5000) return; \
		e  = unmk(e)->next;
	}
	if(e == NULL) {
		at += snprintf(buff+at,5000-at,"(0x0)\n");
	} else if(event::is_one(e)) {
		at += snprintf(buff+at,5000-at,"(0x1)\n");
	} else {
		at += snprintf(buff+at,5000-at,"\n");
	}
	printf("%s",buff);
	printf("%d]   tail = %d/%p\n"
		, ip
		, waiters.tail->con.id
		, waiters.tail->con.item);
}

void
Pool::release(event::Event * & rel_ev, event::Event * by_ev)
{
	// give the resource back
	event::Resource * r = resource;
	assert(*(by_ev->resource_loc) == r);
	rel_ev = waiters.pop(by_ev);
	if(rel_ev) { 
		assert(*(rel_ev->resource_loc) == r);
		assert(rel_ev != by_ev);
		rel_ev->has = index;
		rel_ev->waiting_on = -1;
	}
}

bool
Pool::acquire_or_wait(event::Event * e)
{
	e->waiting_on = index;
	e->has = -1;
	bool acqed = waiters.push(e); // try to acquire the resource
	if(acqed) {
		assert(resource);
		e->waiting_on = -1;
		e->has = index;
	}
	return acqed;
}


} // namespace atomic

size_t num_threads = 2;
size_t num_events_per_thread = 10;
size_t num_atomics = 1000;
size_t iterations = 1000;

pthread_barrier_t barrier;
pthread_barrier_t end_barrier;

atomic::Pool atomics[1024];

#define DUMPATOMICS_ \
	for(size_t i = 0; i < std::min(1u,num_atomics); ++i) { atomics[i].dump(*ip); }

#define _DQ_INTERNAL(RQ) \
   { \
     std::deque<event::Event*>::iterator _qitr = RQ.begin(); \
     std::deque<event::Event*>::iterator _qitr_end = RQ.end(); \
     while(_qitr != _qitr_end) { \
	event::Event * _e = *_qitr; \
	_at += snprintf(_buff + _at,5000-_at,"%d%c,",_e->con.id,(atomics[_e->con.id].resource ? 'r':' ')); \
	if(_at >= 5000) break; \
        ++_qitr; \
     } \
   }
#define DUMPRUNQUEUE_(I) \
   { \
     char _buff[5000]; \
     size_t _at = 0; \
     _at += snprintf(_buff+_at,5000-_at,"%d]  RQ:", *ip); \
     _DQ_INTERNAL(running_evl[I%2]) \
     _DQ_INTERNAL(running_evl[(I+1)%2]) \
     printf("%s\n",_buff); \
   } 
// comment these out to drop most of the I/O:
#define DUMPATOMICS     DUMPATOMICS_
#define dprintf         printf
#define DUMPRUNQUEUE(I) DUMPRUNQUEUE_(I)

void * run_thread(void *vp)
{
	int * ip = reinterpret_cast<int *>(vp);
	long acquire_count = 0;
	long wait_count = 0;
	long release_count = 0;
	long no_op_iterations = 0;

	std::deque<event::Event*> running_evl[2];
	for(size_t i = 0 ; i < num_events_per_thread; ++i) {
		int id = 1+ num_events_per_thread * (*ip) + i;
		running_evl[0].push_back(
			new event::Event(
				  id
				, &(atomics[id].resource)));
	}

	int target_atomic = 0;
	event::Event * e = NULL;
	int id = 0;

	DUMPATOMICS
	// try to distribute to each thread all the atomics
	// this is uncontended acquisition
	for(size_t a = *ip
			; a < num_items && running_evl[0].size() > 0
			; a+= num_threads) {
		e = running_evl[0].front();
		running_evl[0].pop_front();
		id = e->con.id;
		assert( *(e->resource_loc) == NULL);
		if(atomics[id].acquire_or_wait(e)) {
			// got it right away (no competing waiters)
			dprintf("%d]%d}- %d acquired\n",*ip, id,id);
			running_evl[1].push_back(e);
			++acquire_count;
		} else {
			dprintf("%d]%d}- %d waited\n",*ip, id, id);
			++wait_count;
		}
		DUMPATOMICS
		DUMPRUNQUEUE(1)
	}
	
	pthread_barrier_wait(&barrier);
	for(size_t j = 0; j < iterations; ++j) {
		if(running_evl[j%2].size() == 0) {
			++no_op_iterations;
		}
		while(running_evl[j%2].size()) {
			e = running_evl[j%2].front();
			running_evl[j%2].pop_front();
			assert(e->con.id);
			no_op_iterations = 0;
			int a_index = e->con.id;
			dprintf("%d]   %d running {%s\n"
				, *ip
				, e->con.id
				, atomics[a_index].resource ? "r" : "");
			assert(&atomics[a_index].resource == e->resource_loc);
			if(a_index >=0 && atomics[a_index].resource) {
				DUMPATOMICS
				DUMPRUNQUEUE(j)
				dprintf("%d]%d} %d releasing\n",*ip,a_index,e->con.id);
				event::Event *  rel_ev = NULL;
				assert( *(e->resource_loc) != NULL); // && (*(e->resource_loc))->next == NULL);
				atomics[a_index].release(rel_ev,e);
				assert( *(e->resource_loc) == NULL);
				if(rel_ev) {
					running_evl[(j+1)%2].push_back(rel_ev);
					dprintf("%d]%d} %d came out (%p)\n"
						, *ip
						, a_index
						, rel_ev->con.id
						, (*(rel_ev->resource_loc))->con.item);
				} else {
					dprintf("%d]%d} nothing came out\n",*ip,a_index);
				}
				e->has = -1;
				++release_count;
			}
			if(rand()%2) { 
				running_evl[(j+1)%2].push_back(e);
				continue;
			}
			a_index = e->con.id;
			target_atomic = (target_atomic+1)%num_atomics;
			id = e->con.id;
			DUMPATOMICS
			DUMPRUNQUEUE(j)
			assert( *(e->resource_loc) == NULL);
			if(atomics[a_index].acquire_or_wait(e)) {
				assert( *(e->resource_loc) != NULL);
					//&& (*(e->resource_loc))->next == NULL);
				// got it right away (no competing waiters)
				dprintf("%d]%d} %d acquired (%p)\n"
					,*ip, a_index, id, (*(e->resource_loc))->con.item);
				running_evl[(j+1)%2].push_back(e);
				++acquire_count;
			} else {
				dprintf("%d]%d} %d waited\n",*ip,a_index, id);
				++wait_count;
			}
		}
	}
	pthread_barrier_wait(&end_barrier);
	size_t held_count = 0;
	size_t waiting_on = 0;
	std::deque<event::Event*>::iterator qitr = running_evl[iterations%2].begin();
	
	while(qitr != running_evl[iterations%2].end()) {
		event::Event * e = *qitr;
		if(!(e && e->con.id)) {
			break;
		}
		held_count += (e->has >= 0 ? 1 : 0);
		waiting_on += (e->waiting_on >= 0 ? 1 : 0);
		printf("%d]+  %d running with %d\n"
			, *ip, e->con.id, e->has);
		++qitr;
	}
	printf(" thread %d exited ac: %ld wc: %ld rc: %ld "
		"hc: %d wo: %d noi:%ld\n"
		, *ip
		, acquire_count
		, wait_count
		, release_count
		, held_count
		, waiting_on
		, no_op_iterations
		);
	return NULL;
}


int main(int argc, char  * argv[])
{
	if(argc >= 2) {
		num_threads = atoi(argv[1]);
	}
	if(argc >= 3) {
		iterations = atoi(argv[2]);
	}
	srand(iterations);
	if(argc >= 4) {
		num_events_per_thread = atoi(argv[3]);
	}
	if(argc >= 5) {
		num_items = atoi(argv[4]);
	}
	printf("%u threads, %u iterations, %u events/thread, %u resource items\n"
		, num_threads
		, iterations
		, num_events_per_thread
		, num_items);
	// pthread ids
        pthread_t tids[num_threads];
	int tns[num_threads];
	// barriers
	pthread_barrier_init(&barrier,NULL,num_threads);
	pthread_barrier_init(&end_barrier,NULL,num_threads);
	// items
	int r_items[num_items];
	items = &(r_items[0]);
	event::Event dummy_ev(0,0);
	event::Resource * dummy_rs=NULL;
	dummy_ev.resource_loc = &dummy_rs;
	for(size_t i = 0; i < num_items; ++i) {
		// put the initial pool of resources in place
		dummy_rs = new event::Resource(&items[i]);
		atomic::Pool::default_pel.pop(&dummy_ev);
		r_items[i]=i;
	}
	// atomics
	for(size_t a = 0; a < num_atomics; ++a) {
		atomics[a].index = a;
	}
        for(size_t i = 0; i < num_threads; ++i) {
		tns[i] = i;
                int err = pthread_create(&tids[i],NULL,run_thread,&(tns[i])); if(err != 0) {
                        exit(11);
                }
        }
	void * tret = NULL;
	for(size_t i = 0; i < num_threads; ++i) {
		int err = pthread_join(tids[i],&tret);
                if(err != 0) {
                        exit(12);
                }
        }
	__sync_synchronize();
	int ii = -1;
	int  * ip = &ii;
	DUMPATOMICS_ // always
	fflush(stdout);

	return 0;
}

