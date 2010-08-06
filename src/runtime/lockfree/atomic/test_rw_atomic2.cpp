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
#include <lockfree/atomic/OFluxLFAtomicReadWrite.h>
#include <OFluxLogging.h>

namespace event {
	enum {    Read = oflux::atomic::AtomicReadWrite::Read
		, Write = oflux::atomic::AtomicReadWrite::Write
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

struct AtomicReadWrite : public oflux::lockfree::atomic::AtomicReadWrite {
	int index;

	AtomicReadWrite()
		: oflux::lockfree::atomic::AtomicReadWrite(NULL)
	{}
	void dump(int i) 
	{
		printf("for atomic %d:\n",i);
		_dump();
	}
};

AtomicReadWrite atomics[10];

#define str_type(X) (X == event::None ? "None" \
		: (X == event::Read ? "Read" : "Write"))

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


#define _DUMP_A_RUNQUEUE(RQ) \
  { \
    std::deque<EventBasePtr>::iterator _qitr = RQ.begin(); \
    std::deque<EventBasePtr>::iterator _qitr_end = RQ.end(); \
    while(_qitr != _qitr_end) { \
       _at += snprintf(_buff+_at,5000-_at,"%d, ",get_id(*_qitr)); \
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



#define DUMPATOMICS _DUMPATOMICS
#define DUMP_RUNQUEUE(JMOD) _DUMP_RUNQUEUE(JMOD)
#define DUMPATOMIC(A) _DUMPATOMIC(A)
#define dprintf printf

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

	std::deque<EventBasePtr> running_evl[2];

	EventBasePtr ev_src = (*createSrcFn)(oflux::EventBase::no_event,NULL,&n_src);
	// a deque is needed since the ->next relation is in use for this atomic
	for(size_t i = 0 ; i < num_events_per_thread; ++i) {
		EventBasePtr an_ev = (*createNextFn)(ev_src,NULL,&n_next);
		set_id(an_ev,i+1);
		set_has(an_ev,-1);
		set_type(an_ev,event::None);
		running_evl[0].push_back(an_ev);
	}

	int target_atomic = 0;
	EventBasePtr e;

	// try to distribute to each thread all the atomics
	// this is uncontended acquisition
	int id = 0;
	int type = 0;
	for(size_t a = *ip; a < num_atomics; a += num_threads) {
		if(running_evl[0].size() == 0) {
			break;
		}
		e = running_evl[0].front();
		running_evl[0].pop_front();
		id = get_id(e);
		type = oflux::lockfree::atomic::EventBaseHolder::Write;
		set_type(e,type);
		if(atomics[a].acquire_or_wait(e,type)) {
			set_has(e,a);
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
			int a_index = get_has(e);
			int e_id = get_id(e);
			dprintf("%d]   %d running        with %s\n"
				, *ip, e_id, str_type(get_type(e)));
			if(a_index >=0) {
				DUMPATOMIC(a_index);
				//DUMP_RUNQUEUE(j%2);
				dprintf("%d]%d} %d releasing\n"
					,*ip,a_index,e_id);
				std::vector<EventBasePtr> rel_ev;
				atomics[a_index].release(rel_ev,e);
				set_has(e,-1);
				int rel_count = 0;
				for(std::vector<EventBasePtr>::iterator itr = rel_ev.begin()
						; itr != rel_ev.end()
						; ++itr) {
					running_evl[(j+1)%2].push_back(*itr);
					set_has((*itr),a_index);
					dprintf("%d]%d} %d event came out with %s\n"
						, *ip, a_index , get_id(*itr), str_type(get_type(*itr)));
					++release_count[get_type(*itr)];
					++rel_count;
				}
				if(rel_count) {
					dprintf("%d]%d}    %d events out\n"
						, *ip, a_index , rel_count);
				} else {
					dprintf("%d]%d}    nothing out\n",*ip,a_index);
				}
				//e->has = -1; -- hmm
			}
			a_index = target_atomic;
			target_atomic = (target_atomic+1)%num_atomics;
			id = get_id(e);
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
			set_type(e,type);
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
	std::deque<EventBasePtr>::iterator qitr = 
		running_evl[iterations%2].begin();
	std::deque<EventBasePtr>::iterator qitr_end = 
		running_evl[iterations%2].end();
	for(;qitr != qitr_end; ++qitr) {
		held_count += (get_has(*qitr) >= 0 ? 1 : 0);
		//waiting_on += ((*qitr)->waiting_on >= 0 ? 1 : 0); --hmm
		printf("%d]+  %d running        with %s on %d\n"
			,*ip,get_id(*qitr),get_has(*qitr) >=0 ? str_type(get_type(*qitr)) : "    ",get_has(*qitr));
	}
	qitr = running_evl[(iterations+1)%2].begin();
	qitr_end = running_evl[(iterations+1)%2].end();
	for(;qitr != qitr_end; ++qitr) {
		held_count += (get_has(*qitr) >= 0 ? 1 : 0);
		//waiting_on += ((*qitr)->waiting_on >= 0 ? 1 : 0); --hmm
		printf("%d]+  %d running        with %s on %d\n"
			,*ip,get_id(*qitr),get_has(*qitr)>=0 ? str_type(get_type(*qitr)) : "    ",get_has(*qitr));
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

