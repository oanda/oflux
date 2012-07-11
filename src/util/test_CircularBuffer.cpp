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
// building this should be easy:
//  g++ -lrt -g -march=i686 [-DCIRC] [-DNOIO] -o test_<name> test_CircularBuffer.cpp
// where
//  defining CIRC means the program uses the Circular buffer (otherwise the SlowBuffer is used)
//  defining NOIO means that way less (ok no) stdout logging happens


#include "CircularBuffer.h"
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstdio>

struct At {
	At(int ai, int an) : i(ai), n(an) {}
	void run1();
	void run2();

	int i;
	int n;
};

#ifdef NOIO
# define PP(X)
#else
# define PP(X) \
	printf( "%ld.%06ld [%lx] %p " X " (%d,%d)\n" \
		, tv.tv_sec \
		, tv.tv_usec \
		, pthread_self() \
		, this \
		, i \
		, n);
#endif

void
At::run1()
{
	struct timeval tv;
	gettimeofday(&tv,0);
	PP("run1.start");
	usleep(100);
	gettimeofday(&tv,0);
	PP("run1.done");
}


void
At::run2()
{
	struct timeval tv;
	gettimeofday(&tv,0);
	PP("run2.start");
	gettimeofday(&tv,0);
	PP("run2.done");
}


struct SlowBuffer { // not great alternative
	SlowBuffer(const size_t) 
	{
		pthread_mutex_init(&mut,0);
	}
	~SlowBuffer() 
	{
		pthread_mutex_destroy(&mut);
	}
	void submit(At * t) {
		t->run1();
		pthread_mutex_lock(&mut);
		t->run2();
		pthread_mutex_unlock(&mut);
	}

	pthread_mutex_t mut;
};

#ifdef CIRC
typedef oflux::executor::CircularBuffer<At,&At::run1,&At::run2> Buffer;
#else
typedef SlowBuffer Buffer;
#endif

Buffer buffer(10U); // 1024 elements

const size_t num_threads = 10;
const size_t iterations = 20000;
pthread_barrier_t barrier;

void * 
run_thread(void *vp)
{
	int * ip = reinterpret_cast<int *>(vp);
	pthread_barrier_wait(&barrier);
	for(size_t i = 0; i < iterations; ++i) {
		At * t = new At(*ip,i);
		buffer.submit(t);
	}
	return NULL;
}


int 
main()
{
	pthread_barrier_init(&barrier,NULL,num_threads);
        pthread_t tids[num_threads];
	int tns[num_threads];
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
