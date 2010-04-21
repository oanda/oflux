// ./test_rw_atomic 1 193 10 3

#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <deque>
#include <cassert>


namespace event {

enum { None = 0, Read=1, Write=2 };

struct Event {
	Event(int i) 
		: has(-1)
		, waiting_on(-1)
		, id(i)
		, type(0)
		, readrel(false)
		, next(NULL) 
		//, rnext(NULL)
	{}

	int has;
	int waiting_on;
	int id;
	int type;
	bool readrel;
	Event * next;
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

template<typename T>
inline void set_one(T * & p)
{
	reinterpret_cast<size_t &>(p) = 0x0001;
}

template<typename T>
inline bool is_three(T* p)
{
	return reinterpret_cast<size_t>(p) == 0x0003;
}

template<typename T>
inline void set_three(T * & p)
{
	reinterpret_cast<size_t &>(p) = 0x0003;
}


struct RWEventList { // thread safe
	RWEventList() 
		: head(new Event(0))
		, tail(head) 
		, rcount(0)
	{ set_one(head->next); }
	bool push(Event *e, int type);
	void pop(Event * & el, Event * by_e);

	Event * head;
	Event * tail;
	int rcount;
	// state encoding:
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

};

bool
RWEventList::push(Event * e, int type)
{
	e->next = NULL;
	e->readrel = false;
	e->type = None;
	Event * t = NULL;

	int id = e->id;
	int w_on = e->waiting_on;
	int hs = e->has;
	int hi;
	e->id = 0;
	e->waiting_on = -1;
	e->has = -1;
	while(1) {
		Event * h = head;
		Event * hn = h->next;
		int hi = h->id;
		int rc = rcount;
		if(type == Write 
				&& is_one(hn) 
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, 0x0001
				, NULL)) {
			// 1->2
			e->id = id;
			e->waiting_on = w_on;
			e->has = hs;
			e->type = type;
			return true;
		} else if(type == Read
				&& rc == 0
				&& is_one(hn)
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, 0x0001
				, 0x0003)) {
			// 1->3
			__sync_bool_compare_and_swap(
				  &rcount
				, rc
				, 1);
			e->id = id;
			e->waiting_on = w_on;
			e->has = hs;
			e->type = type;
			return true;
		} else if(hn == NULL
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, NULL
				, e)) {
			// 2->4
			h->waiting_on = w_on;
			h->has = hs;
			h->type = type;
			h->id = id;
			return false;
		} else if(is_three(hn)
				&& type == Read
				&& rc > 0
				&& __sync_bool_compare_and_swap(
				  &rcount
				, rc
				, rc+1)) {
			assert(!is_three(e));
			// 3->3
			e->id = id;
			e->waiting_on = w_on;
			e->has = hs;
			e->type = type;
			return true;
		} else if(is_three(hn)
				&& type == Write
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, hn
				, e)) {
			// 3->5
			h->waiting_on = w_on;
			h->has = hs;
			h->type = type;
			h->id = id;
			return false;
		} else if(!is_three(hn) && hn != NULL && !is_one(hn)) {
			// 4->4  or 5->5
			while(tail->next) {
				tail = tail->next;
			}
			t = tail;
			if(head != tail && hi
					&& __sync_bool_compare_and_swap(
						 &(t->next)
						,NULL
						,e)) {
				t->waiting_on = w_on;
				t->has = hs;
				t->type = type;
				t->id = id;
				tail = e;
				break;
			}
		}
	}
	return false;
}

/*
inline void clear_nexts_rlist(Event * & rl)
{
	Event *rp = rl;
	Event *rp_last = NULL;
	while(unmk(rp)) {
		assert(rp->id);
		rp_last = rp;
		rp = rp->rnext;
		rp_last->rnext = NULL;
		rp_last->readrel = false;
	}
}

inline bool mark_reader_done(Event * & rl,Event * e)
{
	Event * r = rl;
#define LOOP_CHECK
#ifdef  LOOP_CHECK
	Event * r2 = rl;
	int i = 0;
#endif
	bool all_rel = true;
	while(unmk(r)) {
		assert(r->id);
		if(r == e) {
			__sync_bool_compare_and_swap(&(r->readrel), false, true);
		}
		all_rel = all_rel && r->readrel;
		r = r->rnext;
#ifdef  LOOP_CHECK
		if(i%2) {
			r2 = r2->rnext;
		}
		assert(r2 != r);
		++i;
#endif
	}
	return rl && rl->readrel && all_rel;
}
*/

void
RWEventList::pop(Event * &el, Event * by_e)
{
	Event * h = NULL;
	Event * hn = NULL;
	int rc = rcount;
	int orig_rc = rc;
	int ht;
	int hi;
	el = NULL;
	assert(by_e->type == Read || rcount == 0);
	assert(by_e->type == Write || rcount > 0);
	bool is_last_reader = 
		( rc > 0
		? __sync_fetch_and_sub(&rcount,1) == 1
		: true);
	while(1) {
		h = head;
		hn = h->next;
		ht = h->type;
		hi = h->id;
		rc = rcount;
		if(hn == NULL
				&& orig_rc == 0
				&& __sync_bool_compare_and_swap(
					  &(head->next)
					, hn
					, 0x0001)) {
			// 2->1
			break;
		} else if(is_three(hn)
				&& orig_rc > 0) {
			if( is_last_reader ) {
				// 3->1
				__sync_bool_compare_and_swap(&(head->next),0x0003,0x0001);
			}
			break;
		} else if(!is_one(hn) && !is_three(hn) && hn != NULL
				&& ht == Write
				&& (is_last_reader || orig_rc ==0)
				&& __sync_bool_compare_and_swap(
					  &head
					, h
					, hn)) {
			// (4,5)->(4,2)
			if(hn->id ==0) {
				tail = head;
			}
			h->next = NULL;
			el = h;
			break;
		} else if(!is_one(hn) && !is_three(hn) && hn != NULL
				&& orig_rc > 0) {
			if(is_last_reader) {
				// 5->(5,3)
				el = NULL;
				while(ht == Read && hi
					&& !is_one(hn) && !is_three(hn) && hn != NULL
					&& __sync_bool_compare_and_swap(
						  &head
						, h
						, hn)) {
					__sync_bool_compare_and_swap(
						  &rcount
						, rc
						, std::min(0,rc-1));
					h->next = el;
					el = h;
					h = head;
					hn = h->next;
					ht = h->type;
					hi = h->id;
					rc = rcount;
				}
			}
			break;
		} else if(!is_one(hn) && !is_three(hn) && hn != NULL
				&& ht == Read
				&& rc == 0) {
			// 4->(5,3)
			el = NULL;
			while(ht == Read && hi
				&& !is_one(hn) && !is_three(hn) && hn != NULL
				&& __sync_bool_compare_and_swap(
					  &head
					, h
					, hn)) {
				if(hn->id == 0) {
					tail = hn;
				}
				h->next = el;
				el = h;
				h = head;
				__sync_fetch_and_add(&rcount,1);
				hn = h->next;
				ht = h->type;
				hi = h->id;
				rc = rcount;
			}
			if(hn == NULL && __sync_bool_compare_and_swap(&(head->next),hn,0x0003)) {}
			break;
		}
	}
}


} // namespace event

namespace atomic {

struct ReadWrite {
	void release(event::Event * & el,event::Event * by_e);
	bool acquire_or_wait(event::Event * e,int);

	void dump();

	event::RWEventList waiters;
	int index; // the ith atomic (lazy that its here)
	char _dontuse[256 - sizeof(int) - sizeof(event::RWEventList)];
		// packing
};

void
ReadWrite::dump()
{
	const char * state =
		(waiters.head->next == NULL ? 
			"exclusive0 [2]"
		: (event::is_one(waiters.head->next) ? 
			"empty      [1]"
		: (event::is_three(waiters.head->next) ? 
			"readingN0  [3]"
		: (!waiters.rcount ? 
			"exclusiveM [4]" : 
			"readingNM  [5]"))));
	printf(" atomic[%d] in state %s\n"
		, index
		, state);
	printf("  head = ");
	event::Event * e = waiters.head;
	while(e != NULL && !event::is_one(e) && !event::is_three(e)) {
		printf("%d%s->",e->id,(e->type == event::Read ? "R" : (e->type == event::Write ? "W" : "")));
		e = e->next;
	}
	if(e == NULL) {
		printf("(0x0)\n");
	} else if(event::is_one(e)) {
		printf("(0x1)\n");
	} else if(event::is_three(e)) {
		printf("(0x3)\n");
	} else {
		printf("\n");
	}
	printf("  tail = %d\n", waiters.tail->id);
	printf("  rcount = %u\n", waiters.rcount);
}


void
ReadWrite::release(event::Event * & el,event::Event * by_e)
{
	assert(by_e->has == index);
	waiters.pop(el,by_e);
	event::Event * e = el;
	while(event::unmk(e)) { 
		e->has = index;
		e->waiting_on = -1;
		e = e->next;
	}
	by_e->has = -1;
}

bool
ReadWrite::acquire_or_wait(event::Event * e, int type)
{
	e->waiting_on = index;
	e->has = -1;
	bool acqed = waiters.push(e,type);
	if(acqed) {
		e->waiting_on = -1;
		e->has = index;
	}
	return acqed;
}


} // namespace atomic

size_t num_threads = 2;
size_t num_events_per_thread = 10;
size_t num_atomics = 2;
size_t iterations = 1000;

pthread_barrier_t barrier;
pthread_barrier_t end_barrier;

atomic::ReadWrite atomics[10];

#define str_type(X) (X == event::None ? "None" \
		: (X == event::Read ? "Read" : "Write"))

void * run_thread(void *vp)
{
	int * ip = reinterpret_cast<int *>(vp);
	long acquire_count[3];
	acquire_count[event::Read] = 0;
	acquire_count[event::Write] = 0;
	long wait_count = 0;
	long release_count[3];
	release_count[event::None] = 0;
	release_count[event::Read] = 0;
	release_count[event::Write] = 0;
	long no_op_iterations = 0;

	std::deque<event::Event *> running_evl[2];
	// a deque is needed since the ->next relation is in use for this atomic
	for(size_t i = 0 ; i < num_events_per_thread; ++i) {
		running_evl[0].push_back(new event::Event(1+ num_events_per_thread * (*ip) + i));
	}

	int target_atomic = 0;
	event::Event * e = NULL;

	// try to distribute to each thread all the atomics
	// this is uncontended acquisition
	e = running_evl[0].front();
	running_evl[0].pop_front();
	int id = e->id;
	int type = event::Write;
	if(atomics[(*ip)%num_atomics].acquire_or_wait(e,type)) {
		// got it right away (no competing waiters)
		printf("%d]%d}- %d acquired       for %s\n",*ip, (*ip)%num_atomics,id,str_type(type));
		running_evl[1].push_back(e);
		++acquire_count[type];
	} else {
		printf("%d]%d}- %d waited         for %s\n",*ip,(*ip)%num_atomics,id,str_type(type));
		++wait_count;
	}
	
	pthread_barrier_wait(&barrier);
	for(size_t j = 0; j < iterations; ++j) {
		printf("------iteration----- %d\n",j);
#define _DUMPATOMICS \
   for(size_t i = 0; i < num_atomics; ++i) { atomics[i].dump(); }
#define DUMPATOMICS //_DUMPATOMICS
		DUMPATOMICS
		if(!running_evl[j%2].size()) {
			++no_op_iterations;
		}
		while(running_evl[j%2].size()) {
			no_op_iterations = 0;
			e = running_evl[j%2].front();
			running_evl[j%2].pop_front();
			int a_index = e->has;
			printf("%d]   %d running        with %s\n",*ip,e->id,str_type(e->type));
			if(a_index >=0) {
				DUMPATOMICS
				printf("%d]%d} %d releasing\n",*ip,a_index,e->id);
				event::Event * rel_ev = NULL;
				atomics[a_index].release(rel_ev,e);
				int rel_count = 0;
				while(event::unmk(rel_ev)) {
					running_evl[(j+1)%2].push_back(rel_ev);
					printf("%d]%d} %d event came out with %s\n"
						, *ip, a_index , rel_ev->id, str_type(rel_ev->type));
					++release_count[rel_ev->type];
					rel_ev = rel_ev->next;
					++rel_count;
				}
				if(rel_count) {
					printf("%d]%d}    %d events out\n"
						, *ip, a_index , rel_count);
				} else {
					printf("%d]%d}    nothing out\n",*ip,a_index);
				}
				e->has = -1;
			}
			a_index = target_atomic;
			target_atomic = (target_atomic+1)%num_atomics;
			id = e->id;
			type = (rand()%2 ? event::Read : event::Write);
			/*if(e->rnext) {
				// limitation of the rnext solution
				// which needs to be fixed once this
				// is not a prototype
				printf("%d]   %d skipped\n",*ip,id);
				running_evl[(j+1)%2].push_back(e);
				continue;
			}*/
			DUMPATOMICS
			if(atomics[a_index].acquire_or_wait(e,type)) {
				// got it right away (no competing waiters)
				printf("%d]%d} %d acquired        for %s\n",*ip,a_index, id,str_type(type));
				running_evl[(j+1)%2].push_back(e);
				++acquire_count[type];
			} else {
				printf("%d]%d} %d waited          for %s\n",*ip,a_index, id,str_type(type));
				++wait_count;
			}
		}
	}
	pthread_barrier_wait(&end_barrier);
	size_t held_count = 0;
	size_t waiting_on = 0;
	std::deque<event::Event *>::iterator qitr = 
		running_evl[iterations%2].begin();
	std::deque<event::Event *>::iterator qitr_end = 
		running_evl[iterations%2].end();
	for(;qitr != qitr_end; ++qitr) {
		held_count += ((*qitr)->has >= 0 ? 1 : 0);
		waiting_on += ((*qitr)->waiting_on >= 0 ? 1 : 0);
		printf("%d]+  %d running        with %s on %d\n",*ip,(*qitr)->id,(*qitr)->has>=0 ? str_type((*qitr)->type) : "    ",(*qitr)->has);
	}
	qitr = running_evl[(iterations+1)%2].begin();
	qitr_end = running_evl[(iterations+1)%2].end();
	for(;qitr != qitr_end; ++qitr) {
		held_count += ((*qitr)->has >= 0 ? 1 : 0);
		waiting_on += ((*qitr)->waiting_on >= 0 ? 1 : 0);
		printf("%d]+  %d running        with %s on %d\n",*ip,(*qitr)->id,(*qitr)->has>=0 ? str_type((*qitr)->type) : "    ",(*qitr)->has);
	}
	printf(" thread %d exited rac: %ld wac:%ld wc: %ld "
		"rrc: %ld wrc:%ld hc: %d wo: %d noi:%ld\n"
		, *ip
		, acquire_count[event::Read]
		, acquire_count[event::Write]
		, wait_count
		, release_count[event::Read]
		, release_count[event::Write]
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
		num_atomics = std::min(10,atoi(argv[4]));
	}
	printf("%d iterations %d threads %d atomics %d events per thread\n"
		, iterations
		, num_threads
		, num_atomics
		, num_events_per_thread);
        pthread_t tids[num_threads];
	int tns[num_threads];
	pthread_barrier_init(&barrier,NULL,num_threads);
	pthread_barrier_init(&end_barrier,NULL,num_threads);
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
	//size_t a_count = 0;
	for(size_t i = 0; i < num_atomics; ++i) {
		atomics[i].dump();
	}
	//printf("program ac: %u\n",a_count);
	fflush(stdout);

	return 0;
}

