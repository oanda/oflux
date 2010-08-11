
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <deque>
#include <cassert>
#include <iostream>
#include <OFlux.h>
#include <flow/OFluxFlowNode.h>
#include <event/OFluxEvent.h>
#include <lockfree/atomic/OFluxLFAtomic.h>
#include <OFluxLogging.h>

#define dprintf printf

namespace event {

	enum {
		Exclusive = oflux::atomic::AtomicExclusive::Exclusive
		, None = oflux::atomic::AtomicCommon::None
	};

} // namespace event

using oflux::EventBasePtr;

class EventData : public oflux::BaseOutputStruct<EventData> {
public:
  typedef EventData base_type;
  int id;
  int has;
  int type;
  int waiting_on;
  bool operator==(const EventData &o) const{ return (o.id==id); }
};

class Empty : public oflux::BaseOutputStruct<Empty> {
public:
        bool operator==(const Empty &) const { return true; }
        typedef Empty base_type;
};

class AtomsEmpty {
public:
        void fill(oflux::atomic::AtomicsHolder * ah) {}
};

namespace oflux {

template<>
inline EventData * convert<EventData>(EventData::base_type *a) { return a; }
template<>
inline const EventData * const_convert<EventData>(const EventData::base_type *a) { return a; }

template<>
inline Empty * convert<Empty>(Empty::base_type *a) { return a; }
template<>
inline const Empty * const_convert<Empty>(const Empty::base_type *a) { return a; }

} // namespace oflux

struct c_srcDetail {
        typedef Empty In_;
        typedef EventData Out_;
        typedef AtomsEmpty Atoms_;
        typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *);
        static nfunctype nfunc;
};

int f_src_func(const Empty *,EventData *, AtomsEmpty*)
{
	return 0;
}

c_srcDetail::nfunctype c_srcDetail::nfunc = &f_src_func;

oflux::CreateNodeFn createSrcFn = oflux::create<c_srcDetail>;

oflux::flow::Node n_src("src",createSrcFn,false,false,false,0,0);

struct c_nextDetail {
        typedef EventData In_;
        typedef EventData Out_;
        typedef AtomsEmpty Atoms_;
        typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *);
        static nfunctype nfunc;
};

int f_next_func(const EventData * ed,EventData *, AtomsEmpty*)
{
	return 0;
}

c_nextDetail::nfunctype c_nextDetail::nfunc = &f_next_func;

oflux::CreateNodeFn createNextFn = oflux::create<c_nextDetail>;

oflux::flow::Node n_next("next",createNextFn,false,false,false,0,0);

//
// using this (stuff above):
//  EventBasePtr ev = (*createNodeFunction)(_another_ev_,NULL,n_node_for_ev);
//   _another_ev_ can be oflux::EventBase::no_event when there is no predecessor
// call
//  ev.execute();

// main program and thread top-level function


size_t num_threads = 2;
size_t num_events_per_thread = 10;
size_t num_atomics = 2;
size_t iterations = 1000;

pthread_barrier_t barrier;
pthread_barrier_t end_barrier;

struct AtomicExclusive : public oflux::lockfree::atomic::AtomicExclusive {
	int index;

	AtomicExclusive()
		: oflux::lockfree::atomic::AtomicExclusive(NULL)
	{}
	void dump(int i = -1) 
	{
		i = (i==-1 ? index : i);
		printf("for atomic %d:\n",i);
		_dump();
	}
};

AtomicExclusive atomics[1024];

inline int
get_id(oflux::EventBasePtr & ev)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        return edptr->id;
}

inline void
set_id(oflux::EventBasePtr & ev, int id)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        edptr->id = id;
}

inline int
get_has(oflux::EventBasePtr & ev)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        return edptr->has;
}
inline void
set_has(oflux::EventBasePtr & ev, int has)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        edptr->has = has;
}

inline int
get_waiting_on(oflux::EventBasePtr & ev)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        return edptr->waiting_on;
}
inline void
set_waiting_on(oflux::EventBasePtr & ev, int wo)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        edptr->waiting_on = wo;
}

inline int
get_type(oflux::EventBasePtr & ev)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        return edptr->type;
}

inline void
set_type(oflux::EventBasePtr & ev, int type)
{
	oflux::OutputWalker ow = ev->output_type();
	EventData * edptr = reinterpret_cast<EventData *>(ow.next());
	assert(edptr);
        edptr->type = type;
}



#define DUMPATOMICS_ \
	for(size_t i = 0; i < num_atomics; ++i) { atomics[i].dump(); }
#define DUMPATOMICS //DUMPATOMICS_

void * run_thread(void *vp)
{
	int * ip = reinterpret_cast<int *>(vp);
	long acquire_count = 0;
	long wait_count = 0;
	long release_count = 0;
	long no_op_iterations = 0;

	std::deque<EventBasePtr> running_evl[2];
	EventBasePtr ev_src = (*createSrcFn)(oflux::EventBase::no_event,NULL,&n_src);

	for(size_t i = 0 ; i < num_events_per_thread; ++i) {
		EventBasePtr an_ev = (*createNextFn)(ev_src,NULL,&n_next);
		set_id(an_ev,i+1);
		set_has(an_ev,-1);
		set_type(an_ev,event::None);
		running_evl[0].push_back(an_ev);
	}

	int target_atomic = 0;
	EventBasePtr e;
	int id = 0;

	// try to distribute to each thread all the atomics
	// this is uncontended acquisition
	for(size_t a = *ip
			; a < num_atomics 
			; a+= num_threads) {
		if(running_evl[0].size() == 0) {
			break;
		}
		e = running_evl[0].front();
		running_evl[0].pop_front();
		id = get_id(e);
		if(atomics[a].acquire_or_wait(e,event::Exclusive)) {
			// got it right away (no competing waiters)
			set_has(e,a);
			dprintf("%d]%d}- %d acquired\n",*ip, (*ip)%num_atomics,id);
			running_evl[1].push_back(e);
			++acquire_count;
		} else {
			dprintf("%d]%d}- %d waited\n",*ip,(*ip)%num_atomics, id);
			++wait_count;
		}
	}
	
	pthread_barrier_wait(&barrier);
	for(size_t j = 0; j < iterations; ++j) {
		if(!running_evl[j%2].size()) {
			++no_op_iterations;
		}
		while(running_evl[j%2].size()) {
			no_op_iterations = 0;
			e=running_evl[j%2].front();
			running_evl[j%2].pop_front();
			int a_index = get_has(e);
			dprintf("%d]   %d running\n",*ip,get_id(e));
			if(a_index >=0) {
				DUMPATOMICS
				dprintf("%d]%d} %d releasing\n",*ip,a_index,get_id(e));
				std::vector<EventBasePtr> rel_ev;
				atomics[a_index].release(rel_ev,e);
				for(std::vector<EventBasePtr>::iterator itr = rel_ev.begin()
						; itr != rel_ev.end()
						; ++itr) {
					running_evl[(j+1)%2].push_back(*itr);
				}
				if(rel_ev.size()) {
					dprintf("%d]%d} %d came out\n",*ip,a_index, get_id(rel_ev.front()));
				} else {
					dprintf("%d]%d} nothing came out\n",*ip,a_index);
				}
				set_has(e,-1);
				++release_count;
			}
			a_index = target_atomic;
			target_atomic = (target_atomic+1)%num_atomics;
			id = get_id(e);
			DUMPATOMICS
			if(atomics[a_index].acquire_or_wait(e, event::Exclusive)) {
				// got it right away (no competing waiters)
				dprintf("%d]%d} %d acquired\n",*ip,a_index, id);
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
	std::deque<EventBasePtr>::iterator qitr = running_evl[iterations%2].begin();
	std::deque<EventBasePtr>::iterator qitr_end = running_evl[iterations%2].end();
	while(qitr != qitr_end  && get_id(*qitr)) {
		e = *qitr;
		held_count += (get_has(e) >= 0 ? 1 : 0);
		waiting_on += (get_waiting_on(e) >= 0 ? 1 : 0);
		printf("%d]+  %d running with %d\n"
			, *ip, get_id(e), get_has(e));
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
	oflux::logging::toStream(std::cout);
	//oflux::logging::logger->setLevelOnOff(oflux::logging::LL_trace,true);
	//oflux::logging::logger->setLevelOnOff(oflux::logging::LL_debug,true);
	//oflux::logging::logger->setLevelOnOff(oflux::logging::LL_info,true);
	oflux::logging::logger->setLevelOnOff(oflux::logging::LL_warn,true);
	oflux::logging::logger->setLevelOnOff(oflux::logging::LL_error,true);
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
		num_atomics = atoi(argv[4]);
	}
	printf("%u threads, %u iterations, %u events/thread, %u resource items\n"
		, num_threads
		, iterations
		, num_events_per_thread
		, num_atomics);
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
	//for(size_t i = 0; i < num_atomics; ++i) {
		//a_count += (atomics[i].waiters.marked() ? 1 : 0);
	//}
	//printf("program ac: %u\n",a_count);
	DUMPATOMICS_ // always
	fflush(stdout);

	return 0;
}

