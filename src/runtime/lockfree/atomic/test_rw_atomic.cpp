//
// state: 
//    empty [1]
//      head->next == 0x1
//      && rcount == 0
// trans:
//  push.1->2  post: head->next == 0x0
//  push.1->3  post: head->next == 0x3
//  pop.(4,5)->(4,2) operates on head
//
// state:
//    exclusive0 [2]
//      head->next = 0x0
//      && rcount = 0
//  push.2->4  post: head->next == e
//  push.3->3  if mistake head->next for 0x3
//  push.4->4  if mistake head->next for > 0x3
//  push.5->5  if mistake head->next for > 0x3
//              might this write head->next to e?
//  pop.(4,5)->(4,2) if mistake head->next > 0x3
//              might write head to hn(perceived)
//
//
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

	inline void check_tail_reachable() {
		bool tail_reachable = false;
		Event * h = head;
		Event * t = tail;
		Event e(0);
		e.next = h;
		Event * h2 = &e;
		int count = 0;
		while(h && !is_one(h) && !is_three(h) && h2 && !is_one(h2) && !-is_three(h2) && h != h2) {
			if(h==t) {
				tail_reachable = true;
			}
			h = h->next;
			if(count%2) {
				h2 = h2->next;
			}
			++count;
		}
		if(h && h==h2) {
			printf("%p head wrapping (maybe)\n",this);
		}
		if(!(tail_reachable || head==tail)) {
			printf("%p head cannot reach tail(maybe)\n",this);
		}
		e.next = t;
		Event * t2 = &e;
		count = 0;
		while(t && !is_one(t) && !is_three(t) && t2 && !is_one(t2) && !is_three(t2) && t!=t2) {
			t=t->next;
			if(count%2) {
				t2 = t2->next;
			}
		}
		if(t && t==t2) {
			printf("%p tail wrapping (maybe)\n",this);
		}
	}

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

// push(Event *e,int type) __________________________________________________
// return true and e acquires atomic with type type
//  or
// return false and e must wait/park (as a type type)
bool
RWEventList::push(Event * e, int type)
{
	check_tail_reachable();
	e->next = NULL;
	e->readrel = false;
	e->type = None;
	Event * t = NULL;

	int id = e->id;
	int w_on = e->waiting_on;
	int has = e->has;
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
				&& rc == 0
				&& is_one(hn) 
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, 0x0001
				, NULL)) {
			// 1->2
			e->id = id;
			e->waiting_on = w_on;
			e->has = has;
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
			e->has = has;
			e->type = type;
			return true;
		} else if(hn == NULL
				&& rc == 0
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, NULL
				, e)) {
			// 2->4
			tail = e;
			h->waiting_on = w_on;
			h->has = has;
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
			e->has = has;
			e->type = type;
			return true;
		} else if(is_three(hn)
				&& type == Write
				&& __sync_bool_compare_and_swap(
				  &(head->next)
				, hn
				, e)) {
			// 3->5
			tail = e;
			h->waiting_on = w_on;
			h->has = has;
			h->type = type;
			h->id = id;
			return false;
		} else if(!is_three(hn) 
				&& hn != NULL 
				&& !is_one(hn)) {
			// 4->4  or 5->5
			t = tail;
			while(t && !is_three(t) 
					&& !is_one(t) 
					&& t->next 
					&& !is_three(t->next) 
					&& !is_one(t->next)) {
				t = t->next;
			}
			if(h != t && hi 
					&& h == head
					&& __sync_bool_compare_and_swap(
						  &(t->next)
						, NULL
						, e)) {
				tail = e;
				t->waiting_on = w_on;
				t->has = has;
				t->type = type;
				t->id = id;
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

// pop(Event * & el, Event * by_e) __________________________________________
// by_e releases its hold on the atomic
//  and as a side-effect can output a non-NULL el
//  which is released to run on the atomic
void
RWEventList::pop(Event * &el, Event * by_e)
{
	check_tail_reachable();
	Event * h = NULL;
	Event * hn = NULL;
	int rc;
	int orig_rc;
	int ht;
	int hi;
	el = NULL;

	if(by_e->type == Read) {
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
	assert(by_e->type == Read || orig_rc == 0);
	assert(by_e->type == Write || orig_rc > 0);
	assert(head->next || rc == 0);
	bool is_last_reader = (rc == 0) && (orig_rc == 1);
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
				&& orig_rc > 0
				&& is_last_reader 
				// 3->1
				&& __sync_bool_compare_and_swap(
					  &(head->next)
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
				&& ht == Write
				&& (is_last_reader || orig_rc ==0)
				&& __sync_bool_compare_and_swap(
					  &head
					, h
					, hn)) {
			// (4,5)->(4,2)
			/*if(hn->id ==0 || tail == h) {
				tail = head;
				check_tail_reachable();
			}*/
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
				while(ht == Read 
					&& hi
					&& !is_one(hn) 
					&& !is_three(hn) 
					&& hn != NULL
					&& __sync_bool_compare_and_swap(
						  &head
						, h
						, hn)) {
					h->next = el;
					el = h;
					/*if(h == tail) {
						tail = head;
						check_tail_reachable();
					}*/
					h = head;
					hn = h->next;
					ht = h->type;
					hi = h->id;
					++local_reader_count;
				}
				__sync_fetch_and_add(&rcount,local_reader_count);
				if(el && hn == NULL 
					&& __sync_bool_compare_and_swap(
						  &(head->next)
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
				&& ht == Read
				&& rc == 0) {
			// 4->(5,3)
			el = NULL;
			int local_reader_count = 0;
			while(ht == Read 
				&& hi
				&& !is_one(hn) 
				&& !is_three(hn) 
				&& hn != NULL
				&& __sync_bool_compare_and_swap(
					  &head
					, h
					, hn)) {
				/*if(hn->id == 0) {
					//tail = hn;
					tail = head;
					check_tail_reachable();
				}*/
				h->next = el;
				el = h;
				/*if(h == tail) {
					tail = head;
					check_tail_reachable();
				}*/
				h = head;
				hn = h->next;
				ht = h->type;
				hi = h->id;
				rc = rcount;
				++local_reader_count;
			}
			__sync_fetch_and_add(&rcount,local_reader_count);
			if(el && hn == NULL 
				&& __sync_bool_compare_and_swap(
					  &(head->next)
					, hn
					, 0x0003)) {}
			if(el) {
				// otherwise nothing happened
				break;
			}
		}
	}
}


} // namespace event

namespace atomic {

struct ReadWrite {
	void release(event::Event * & el,event::Event * by_e);
	bool acquire_or_wait(event::Event * e,int);

	void dump(int ip);

	event::RWEventList waiters;
	int index; // the ith atomic (lazy that its here)
	char _dontuse[256 - sizeof(int) - sizeof(event::RWEventList)];
		// packing
};

void
ReadWrite::dump(int ip)
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
	printf("%d] atomic[%d] in state %s\n"
		, ip
		, index
		, state);
	size_t at = 0;
	char buff[5000];
	at += snprintf(buff+at,5000-at,"%d]  head = ",ip);
	event::Event * e = waiters.head;
	while(e != NULL && !event::is_one(e) && !event::is_three(e)) {
		at += snprintf(buff+at,5000-at,"%d%s->",e->id,(e->type == event::Read ? "R" : (e->type == event::Write ? "W" : "")));
		e = e->next;
	}
	if(e == NULL) {
		at += snprintf(buff+at,5000-at,"(0x0)\n");
	} else if(event::is_one(e)) {
		at += snprintf(buff+at,5000-at,"(0x1)\n");
	} else if(event::is_three(e)) {
		at += snprintf(buff+at,5000-at,"(0x3)\n");
	} else {
		at += snprintf(buff+at,5000-at,"\n");
	}
	printf("%s",buff);
	printf("%d]  tail = %d\n", ip, waiters.tail->id);
	printf("%d]  rcount = %u\n", ip, waiters.rcount);
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

#define _DUMP_A_RUNQUEUE(RQ) \
  { \
    std::deque<event::Event *>::iterator _qitr = RQ.begin(); \
    std::deque<event::Event *>::iterator _qitr_end = RQ.end(); \
    while(_qitr != _qitr_end) { \
       _at += snprintf(_buff+_at,5000-_at,"%d, ",(*_qitr)->id); \
       ++_qitr; \
    } \
  }

#define _DUMP_RUNQUEUE(JMOD) \
  { \
    char _buff[5000]; \
    size_t _at = 0; \
    _at += snprintf(_buff+_at,5000-_at,"%d]  RQ: ",*ip); \
    _DUMP_A_RUNQUEUE(running_evl[JMOD]); \
    _DUMP_A_RUNQUEUE(running_evl[(JMOD) ? 0 : 1]); \
    printf("%s\n",_buff); \
  }

#define _DUMPATOMICS \
   for(size_t i = 0; i < num_atomics; ++i) { atomics[i].dump(*ip); }
#define _DUMPATOMIC(A) atomics[A].dump(*ip);



#define DUMPATOMICS //_DUMPATOMICS
#define DUMP_RUNQUEUE(JMOD) //_DUMP_RUNQUEUE(JMOD)
#define DUMPATOMIC(A) //_DUMPATOMICS(A)
#define dprintf //printf

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
	int id = 0;
	int type = 0;
	for(int a = *ip; a < num_atomics; a += num_threads) {
		if(running_evl[0].size() == 0) {
			break;
		}
		e = running_evl[0].front();
		running_evl[0].pop_front();
		id = e->id;
		type = event::Write;
		if(atomics[a].acquire_or_wait(e,type)) {
			// got it right away (no competing waiters)
			printf("%d]%d}- %d acquired       for %s\n",*ip,a,id,str_type(type));
			running_evl[1].push_back(e);
			++acquire_count[type];
		} else {
			printf("%d]%d}- %d waited         for %s\n",*ip,a,id,str_type(type));
			++wait_count;
		}
	}
	
	pthread_barrier_wait(&barrier);
	for(size_t j = 0; j < iterations; ++j) {
		dprintf("%d] ------iteration----- %d\n",*ip,j);
		//DUMPATOMICS;
		DUMP_RUNQUEUE(j%2);
		if(!running_evl[j%2].size()) {
			++no_op_iterations;
		}
		while(running_evl[j%2].size()) {
			no_op_iterations = 0;
			e = running_evl[j%2].front();
			running_evl[j%2].pop_front();
			int a_index = e->has;
			dprintf("%d]   %d running        with %s\n",*ip,e->id,str_type(e->type));
			if(a_index >=0) {
				DUMPATOMIC(a_index);
				//DUMP_RUNQUEUE(j%2);
				dprintf("%d]%d} %d releasing\n",*ip,a_index,e->id);
				event::Event * rel_ev = NULL;
				atomics[a_index].release(rel_ev,e);
				int rel_count = 0;
				while(event::unmk(rel_ev)) {
					running_evl[(j+1)%2].push_back(rel_ev);
					dprintf("%d]%d} %d event came out with %s\n"
						, *ip, a_index , rel_ev->id, str_type(rel_ev->type));
					++release_count[rel_ev->type];
					rel_ev = rel_ev->next;
					++rel_count;
				}
				if(rel_count) {
					dprintf("%d]%d}    %d events out\n"
						, *ip, a_index , rel_count);
				} else {
					dprintf("%d]%d}    nothing out\n",*ip,a_index);
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
			DUMPATOMIC(a_index);
			DUMP_RUNQUEUE(j%2);
			if(atomics[a_index].acquire_or_wait(e,type)) {
				// got it right away (no competing waiters)
				dprintf("%d]%d} %d acquired        for %s\n",*ip,a_index, id,str_type(type));
				running_evl[(j+1)%2].push_back(e);
				++acquire_count[type];
			} else {
				dprintf("%d]%d} %d waited          for %s\n",*ip,a_index, id,str_type(type));
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
	printf("%d threads %d iterations %d atomics %d events per thread\n"
		, num_threads
		, iterations
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
		atomics[i].dump(-1);
	}
	//printf("program ac: %u\n",a_count);
	fflush(stdout);

	return 0;
}

