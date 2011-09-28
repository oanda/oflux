
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <deque>
#include <pthread.h>
#include "OFluxWorkStealingDeque.h"

using namespace oflux::lockfree;

size_t num_threads = 2;
size_t num_events_per_thread = 10;
size_t num_steals = num_events_per_thread;
size_t iterations = 1000;

pthread_barrier_t barrier;
pthread_barrier_t end_barrier;

struct Event {
	Event() : id(0), content(0) {}
	Event(int i) : id(i), content(0) {}
	int id;
	int content;
};

CircularWorkStealingDeque<Event> ws_queues[16];

void * 
run_thread (void *vp)
{
	int * ip = reinterpret_cast<int *>(vp);
	long total_event_executions = 0;
	long total_steal_successes = 0;
	long total_steal_failures = 0;

	CircularWorkStealingDeque<Event> & deque = ws_queues[*ip];
	// startup phase
	for(size_t i = 0 ; i < num_events_per_thread; ++i) {
		deque.pushBottom(new Event(1+ num_events_per_thread * (*ip) + i));
	}

	pthread_barrier_wait(&barrier);
	// running phase
	for(size_t i = 0; i < iterations; ++i) {
		Event * e = NULL;
		size_t num_executed = 0;
		std::deque<Event*> local_events;
		while((e = deque.popBottom()) != CircularWorkStealingDeque<Event>::empty && num_executed < num_events_per_thread+1) {
			assert(!e->content);
			e->content = *ip;
			local_events.push_back(e);
			++num_executed;
		}
		while(local_events.size() > 0) {
			e = local_events.front();
			local_events.pop_front();
			e->content = 0;
			deque.pushBottom(e);
			++total_event_executions;
		}
		for(size_t j = num_executed ; j < num_events_per_thread+num_steals; ++j) {
			int which = j%num_threads;
			++num_executed;
			if((*ip) == which) continue; // no self stealing
			e = deque.steal();
			if(e == CircularWorkStealingDeque<Event>::empty
					|| e == CircularWorkStealingDeque<Event>::abort) {
				++total_steal_failures;
				continue;
			}
			++total_steal_successes;
			++total_event_executions;
			deque.pushBottom(e);
		}
	}

	pthread_barrier_wait(&end_barrier);
	// reporting phase
	printf(" thread %d exited events: %ld steal[successes]: %ld steal[failures]: %ld\n"
		, *ip
		, total_event_executions
		, total_steal_successes
		, total_steal_failures
		);
	return NULL;
}


int
main(int argc,char * argv[])
{
	if(argc >= 2) {
		num_threads = std::min(16,atoi(argv[1]));
	}
	if(argc >= 3) {
		iterations = atoi(argv[2]);
	}
	srand(iterations);
	if(argc >= 4) {
		num_events_per_thread = atoi(argv[3]);
	}
	if(argc >= 5) {
		num_steals = atoi(argv[4]);
	}
	printf("%d iterations %d threads %d events per thread %d num steals per iteration\n"
		, iterations
		, num_threads
		, num_events_per_thread
		, num_steals
		);
        pthread_t tids[num_threads];
	int tns[num_threads];
	pthread_barrier_init(&barrier,NULL,num_threads);
	pthread_barrier_init(&end_barrier,NULL,num_threads);
        for(size_t i = 0; i < num_threads; ++i) {
		tns[i] = i;
                int err = pthread_create(&tids[i],NULL,run_thread,&(tns[i])); 
		if(err != 0) {
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
	fflush(stdout);

	return 0;
}

