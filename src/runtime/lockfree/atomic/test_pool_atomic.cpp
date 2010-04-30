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
	Event(int i) : has(-1), waiting_on(-1), next(NULL) 
	{ con.id = i; }

	Event(int * it) : has(-1), waiting_on(-1), next(NULL)
	{ con.item = it; }

	int has;
	int waiting_on;
	union Con { 
		int id;     // id of event
		int * item; // ptr to resource for storing Resources
	} con;
	Event * captured_item; // for events
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
	PoolEventList() : head(new Event(0)), tail(head) 
	{}
	Resource * push(Event *e); // acquire a pool item
		// (when successful event->captured_item is a resource

	Event * pop(Resource * r); //release a pool item

	inline bool marked() const 
	{ return is_one(head->next); }

	Event * head;
	Event * tail;
	// state encoding:
	//
	// 1. (resourcesN) !mkd(head) && head = { next != NULL, id > 0 }
	// 2. (empty) !mkd(head) && head = { next == NULL, id == 0 }
	// 3. (waitingM) mkd(head) && unmkd(head) = { mkd(next) , next != NULL, id > 0 }
	// Note: in (3) all non-NULL next links are mkd()
};

Resource *
PoolEventList::push(Event * e)
{
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
		while(tail->next) {
			tail = tail->next;
		}
		t = tail;
		if(	// condition:
			!mkd(h)
			&& hn != NULL
			// action:
			&& __sync_bool_compare_and_swap(&head,h,hn)
			) {
			// 1->(1,2)
			return h;
		} else if( //condition
			!mkd(h)
			&& !hn
			// action:
			&& (e->next = h)
			&& __sync_bool_compare_and_swap(&head,h,mk(e))
			) {
			// 2->3
			return NULL;
		} else if( //condition:
			mkd(h)
			&& unmk(hn)
			// action:
			&& (e->next = mk(h))
			&& __sync_bool_compare_and_swap(&head,h,mk(e))
			) {
			// 3->3
			// parking it on the head (not starvation free!)
			return NULL;
		}
	}
	return NULL;
}

Event *
PoolEventList::pop(Resource * r)
{
	Event * h = NULL;
	Event * hn = NULL;
	while(1) {
		h = head;
		hn = unmk(h)->next;
		if(	// condition:
			!mkd(h)
			&& (!hn || h->con.item != NULL)
			// action:
			&& (r->next = h)
			&& __sync_bool_compare_and_swap(&head,h,r)) {
			// (1,2)->1
			return NULL;
		} else if(
			// condition:
			mkd(h)
			&& unmk(h)->con.id > 0
			&& unmk(hn) != NULL
			// action:
			&& __sync_bool_compare_and_swap(&head,h,unmk(hn)->con.id > 0 ? mk(hn) : unmk(hn))) {
			// 3->(2,3)
			h->next = NULL;
			return unmk(h);
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
	{}
	void release(event::Event * & rel_ev);
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
			"waitingM   [3]"
		: (waiters.head->next == NULL ?
			"empty      [2]" :       
			"resourcesN [1]")));

	printf(" atomic[%d] in state %s\n"
		, index
		, state);
	printf(" %s head = ", mkd(waiters.head) ? "m" : " ");
	event::Event * e = waiters.head;
	while(e != NULL) {
		if(mkd(e)) {
			printf("%d->",unmk(e)->con.id);
		} else {
			printf("%p->",unmk(e)->con.item);
		}
		e  = unmk(e)->next;
	}
	if(e == NULL) {
		printf("(0x0)\n");
	} else if(event::is_one(e)) {
		printf("(0x1)\n");
	} else {
		printf("\n");
	}
	printf("   tail = %d/%p\n"
		, waiters.tail->con.id
		, waiters.tail->con.item);
}

void
Pool::release(event::Event * & rel_ev)
{
	// give the resource back
	rel_ev = waiters.pop(resource);
	if(rel_ev) { 
		rel_ev->captured_item = resource;
		rel_ev->has = index;
		rel_ev->waiting_on = -1;
	}
	resource = NULL;
}

bool
Pool::acquire_or_wait(event::Event * e)
{
	e->waiting_on = index;
	e->has = -1;
	resource = waiters.push(e); // try to acquire the resource
	if(resource) {
		e->captured_item = resource;
		e->waiting_on = -1;
		e->has = index;
	}
	return resource != NULL;
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
#define DUMPATOMICS DUMPATOMICS_

#define _DQ_INTERNAL \
     while(_e) { \
	printf("%d%c,",_e->con.id,(_e->captured_item ? 'r':' ')); \
       _e = _e->next; \
     } 
#define DUMPRUNQUEUE(I) \
   { \
     printf("  RQ:"); \
     event::Event * _e = running_evl[I%2].head; \
     _DQ_INTERNAL \
     _e = running_evl[(I+1)%2].head; \
     _DQ_INTERNAL \
     printf("\n"); \
   } 

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
		running_evl[0].push(new event::Event(1+ num_events_per_thread * (*ip) + i));
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
		if(atomics[id].acquire_or_wait(e)) {
			// got it right away (no competing waiters)
			printf("%d]%d}- %d acquired\n",*ip, id,id);
			running_evl[1].push(e);
			++acquire_count;
		} else {
			printf("%d]%d}- %d waited\n",*ip, id, id);
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
			no_op_iterations = 0;
			int a_index = e->con.id;
			printf("%d]   %d running {%s\n"
				, *ip
				, e->con.id
				, e->captured_item ? "r" : "");
			assert(atomics[a_index].resource == e->captured_item);
			if(a_index >=0 && atomics[a_index].resource) {
				DUMPATOMICS
				DUMPRUNQUEUE(j)
				printf("%d]%d} %d releasing\n",*ip,a_index,e->con.id);
				event::Event * oldtail = running_evl[(j+1)%2].tail;
				event::Event *  rel_ev = NULL;
				e->captured_item = NULL;
				atomics[a_index].release(rel_ev);
				if(rel_ev) {
					atomics[rel_ev->con.id].resource = rel_ev->captured_item;
					running_evl[(j+1)%2].push(rel_ev);
				}
				if(running_evl[(j+1)%2].tail && running_evl[(j+1)%2].tail != oldtail) {
					printf("%d]%d} %d came out\n",*ip,a_index, running_evl[(j+1)%2].tail->con.id);
				} else {
					printf("%d]%d} nothing came out\n",*ip,a_index);
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
			if(atomics[a_index].acquire_or_wait(e)) {
				// got it right away (no competing waiters)
				printf("%d]%d} %d acquired\n",*ip,a_index, id);
				running_evl[(j+1)%2].push(e);
				++acquire_count;
			} else {
				printf("%d]%d} %d waited\n",*ip,a_index, id);
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
	for(size_t i = 0; i < num_items; ++i) {
		// put the initial pool of resources in place
		atomic::Pool::default_pel.pop(new event::Resource(&items[i]));
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
	//for(size_t i = 0; i < num_atomics; ++i) {
		//a_count += (atomics[i].waiters.marked() ? 1 : 0);
	//}
	//printf("program ac: %u\n",a_count);
	DUMPATOMICS_ // always
	fflush(stdout);

	return 0;
}

