// working on:
// ./test_pool_atomic 2 7 2 1


#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include <cassert>

size_t num_items = 5;
int * items = NULL;

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
	PoolEventList() : head(new Event(0,0)), tail(head) 
	{}
	bool push(Event *e); // acquire a pool item

	Event * pop(Resource * r); //release a pool item

	Event * head;
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
	e->next = NULL;
	Event * t = NULL;

	//int id = e->con.id;
	//int w_on = e->waiting_on;
	//int hs = e->has;
	//e->con.id = 0;
	//e->waiting_on = -1;
	//e->has = -1;
	Event * h = NULL;
	Event * hn = NULL;
	while(1) {
		h = head;
		hn = unmk(h)->next;
		/* not used currently:
		t = tail;
		while(t->next) {
			t = t->next;
		}
		*/
		if(	// condition:
			mkd(h)
			&& unmk(hn) != NULL) {
			// action:
			*(e->resource_loc) = unmk(h);
			if(__sync_bool_compare_and_swap(&head,h,unmk(hn)->con.item ? hn : unmk(hn))
				) {
				// 1->(1,2)
				unmk(h)->next = NULL;
				e->next = NULL;
				return true;
			}
			*(e->resource_loc) = NULL;
		} else if( //condition
			h
			&& !mkd(h)
			&& !hn) {
			// action:
			e->next = h;
			if(__sync_bool_compare_and_swap(&head,h,e)) {
				// 2->3
				return false;
			}
			e->next = NULL;
		} else if( //condition:
			h 
			&& !mkd(h)
			&& hn) {
			// action:
			e->next = h;
			if(__sync_bool_compare_and_swap(&head,h,e)) {
				// 3->3
				// parking it on the head (not starvation free!)
				// TODO: stop starving
				return false;
			}
			e->next = NULL;
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
	Event * h = NULL;
	Event * hn = NULL;
	while(1) {
		h = head;
		hn = unmk(h)->next;
		if(	// condition:
			mkd(h)
			&& (!hn || h->con.item != NULL)) {
			// action:
			r->next = mk(h);
			if(__sync_bool_compare_and_swap(&head,h,mk(r))) {
				// 1->1
				return NULL;
			}
			r->next = NULL;
		} else if(
			// condition:
			!mkd(h)
			&& !hn) {
			// action:
			r->next = mk(h);
			if(__sync_bool_compare_and_swap(&head,h,mk(r))) {
				// 2->1
				return NULL;
			}
			r->next = NULL;
		} else if(
			// condition:
			!mkd(h)
			&& h->con.id > 0
			&& unmk(hn) != NULL) {
			// action:
			if(__sync_bool_compare_and_swap(&head,h,unmk(hn))) {
				// 3->(2,3)
				h->next = NULL;
				*(h->resource_loc) = r;
				r->next = NULL;
				return h;
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

	void dump();

	event::PoolEventList & waiters;

	event::Resource * resource;
	int index;
};

event::PoolEventList Pool::default_pel;


void
Pool::dump()
{
	const char * state =
		( waiters.head == 0 ?
			"undefined  [0]"
		: (event::mkd(waiters.head) ?
			"resourcesN [1]"
		: (waiters.head->next == NULL ?
			"empty      [2]" :       
			"waitingM   [3]" )));

	printf(" atomic[%d] in state %s\n"
		, index
		, state);
	char buff[5000];
	size_t at = 0;
	at += snprintf(buff+at,5000-at," %s head = ", mkd(waiters.head) ? "m" : " ");
	event::Event * e = waiters.head;
	while(unmk(e) != NULL) {
		if(!mkd(e)) {
			at += snprintf(buff+at,5000-at,"%c%d->",mkd(e) ? 'm' : ' ',unmk(e)->con.id);
		} else {
			at += snprintf(buff+at,5000-at,"%c%p->",mkd(e) ? 'm' : ' ',unmk(e)->con.item);
		}
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
	printf("   tail = %d/%p\n"
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
	for(size_t i = 0; i < std::min(1u,num_atomics); ++i) { atomics[i].dump(); }

#define _DQ_INTERNAL \
     while(_e) { \
	_at += snprintf(_buff + _at,5000-_at,"%d%c,",_e->con.id,(atomics[_e->con.id].resource ? 'r':' ')); \
       _e = _e->next; \
     } 
#define DUMPRUNQUEUE_(I) \
   { \
     char _buff[5000]; \
     size_t _at = 0; \
     _at += snprintf(_buff+_at,5000-_at,"%d]  RQ:", *ip); \
     event::Event * _e = running_evl[I%2].head; \
     _DQ_INTERNAL \
     _e = running_evl[(I+1)%2].head; \
     _DQ_INTERNAL \
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

	event::LocalEventList running_evl[2];
	//event::LocalEventList * from_other_thread = NULL;
	for(size_t i = 0 ; i < num_events_per_thread; ++i) {
		int id = 1+ num_events_per_thread * (*ip) + i;
		running_evl[0].push(
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
			; a < num_items && running_evl[0].head != NULL
			; a+= num_threads) {
		e = running_evl[0].pop();
		id = e->con.id;
		assert( *(e->resource_loc) == NULL);
		if(atomics[id].acquire_or_wait(e)) {
			// got it right away (no competing waiters)
			dprintf("%d]%d}- %d acquired\n",*ip, id,id);
			running_evl[1].push(e);
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
		if(!running_evl[j%2].head) {
			++no_op_iterations;
		}
		while(e = running_evl[j%2].pop()) {
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
				event::Event * oldtail = running_evl[(j+1)%2].tail;
				event::Event *  rel_ev = NULL;
				assert( *(e->resource_loc) != NULL && (*(e->resource_loc))->next == NULL);
				atomics[a_index].release(rel_ev,e);
				assert( *(e->resource_loc) == NULL);
				if(rel_ev) {
					running_evl[(j+1)%2].push(rel_ev);
				}
				if(running_evl[(j+1)%2].tail && running_evl[(j+1)%2].tail != oldtail) {
					dprintf("%d]%d} %d came out (%p)\n"
						, *ip
						, a_index
						, running_evl[(j+1)%2].tail->con.id
						, (*(running_evl[(j+1)%2].tail->resource_loc))->con.item);
				} else {
					dprintf("%d]%d} nothing came out\n",*ip,a_index);
				}
				e->has = -1;
				++release_count;
			}
			if(rand()%2) { 
				running_evl[(j+1)%2].push(e);
				continue;
			}
			a_index = e->con.id;
			target_atomic = (target_atomic+1)%num_atomics;
			id = e->con.id;
			DUMPATOMICS
			DUMPRUNQUEUE(j)
			assert( *(e->resource_loc) == NULL);
			if(atomics[a_index].acquire_or_wait(e)) {
				assert( *(e->resource_loc) != NULL
					&& (*(e->resource_loc))->next == NULL);
				// got it right away (no competing waiters)
				dprintf("%d]%d} %d acquired (%p)\n"
					,*ip, a_index, id, (*(e->resource_loc))->con.item);
				running_evl[(j+1)%2].push(e);
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
	e = running_evl[iterations%2].head;
	while(e && e->con.id) {
		held_count += (e->has >= 0 ? 1 : 0);
		waiting_on += (e->waiting_on >= 0 ? 1 : 0);
		printf("%d]+  %d running with %d\n"
			, *ip, e->con.id, e->has);
		e = e->next;
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
	size_t a_count = 0;
	__sync_synchronize();
	DUMPATOMICS_ // always
	fflush(stdout);

	return 0;
}

