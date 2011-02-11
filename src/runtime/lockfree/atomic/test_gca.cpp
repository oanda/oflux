#include <pthread.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <deque>
#include <cassert>
#include <climits>
#include <iostream>
#include <algorithm>
#include "OFluxGrowableCircularArray.h"
//#include <OFluxLogging.h>


oflux::lockfree::growable::LFArrayQueue<int> q;

size_t sleep_time = 5;
size_t num_threads = 2;
pthread_barrier_t barrier;
pthread_barrier_t end_barrier;

size_t ramp = 1000;

void * run_thread(void *vp)
{
	int * ip = reinterpret_cast<int *>(vp);
	struct timeval lasttime;
	gettimeofday(&lasttime, NULL);
	long push_count = 0;
	long pop_count = 0;
	const long over = 2000 * std::min(1000U,ramp); //000;
	struct timeval nowtime;

	while(1) {
		if(push_count > over || pop_count > over) {
			gettimeofday(&nowtime,NULL);
			double sec =
				(nowtime.tv_sec - lasttime.tv_sec)
				+ (nowtime.tv_usec - lasttime.tv_usec)/1000000.0;
			printf("[%3d] pushed %lf per sec\n"
				, *ip
				, ((double)push_count)/sec);
			printf("[%3d] popped %lf per sec\n"
				, *ip
				, ((double)pop_count)/sec);
			gettimeofday(&lasttime, NULL);
			assert(pop_count > 0);
			push_count = 0;
			pop_count = 0;
		}
		size_t num = rand() % ramp;
		union {
			int * ptr;
			struct { char tid; char item; } s;
		} u;
		for(size_t i = 0; i < num; ++i) {
			u.s.item = i+1;
			u.s.tid = (*ip << 4) | ((i+1) >> 8);
			//printf("[%3d] push %10p (%d,%d)\n"
				//, *ip, u.ptr, *ip, i+1);
			q.push(u.ptr);
			++push_count;
		}
		usleep(sleep_time);
		for(size_t i = 0; i < num && u.ptr; ++i) {
			u.ptr = q.pop();
			pop_count += (u.ptr ? 1 : 0);
			//printf("[%3d] pop %10p\n",*ip, u.ptr);
		}
		usleep(sleep_time);
	}
	return NULL;
}



int main(int argc, char  * argv[])
{
	/*
	oflux::logging::toStream(std::cout);
	//oflux::logging::logger->setLevelOnOff(oflux::logging::LL_trace,true);
	//oflux::logging::logger->setLevelOnOff(oflux::logging::LL_debug,true);
	//oflux::logging::logger->setLevelOnOff(oflux::logging::LL_info,true);
	oflux::logging::logger->setLevelOnOff(oflux::logging::LL_warn,true);
	oflux::logging::logger->setLevelOnOff(oflux::logging::LL_error,true);
	*/
	if(argc >= 2) {
		num_threads = atoi(argv[1]);
	}
	if(argc >= 3) {
		sleep_time = atoi(argv[2]);
	}
	if(argc >= 4) {
		ramp = atoi(argv[3]);
	}
	printf("%u threads, %u sleep time %u ramp\n"
		, num_threads
		, sleep_time
		, ramp);
        pthread_t tids[num_threads];
	int tns[num_threads];
	pthread_barrier_init(&barrier,NULL,num_threads);
	pthread_barrier_init(&end_barrier,NULL,num_threads);
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
	fflush(stdout);

	return 0;
}
