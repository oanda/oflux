#include "OFluxGenerate_sleeppipe.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#define START_USLEEP 1000 // usec

int tn = 0;

void copytime(mtime_t &in, const mtime_t from)
{
	in.tv_sec = from.tv_sec;
	in.tv_usec = from.tv_usec;
}

int Start(const Start_in *, Start_out * so, Start_atoms *)
{
	tn++;
	so->transaction_number = tn;
	usleep(START_USLEEP);
	gettimeofday(&(so->start_t), NULL);
	return 0; // meaning spawn
}

#define SLEEP_SCALE 1000

int Sleep(const Sleep_in *si, Sleep_out * so, Sleep_atoms *)
{
	useconds_t st = 10 * ((int)(rand()*((double)SLEEP_SCALE)/RAND_MAX));
	usleep(st);
	so->sleep_req = (int)st;
	so->transaction_number = si->transaction_number;
	gettimeofday(&(so->sleep_t),NULL);
	copytime(so->start_t,si->start_t);
	return 0;
}

// CPU eater
int fib(int n) 
{
	return ( n <= 1 ? 1 : fib(n-1) + fib(n-2) );
}

#define FIB_WORK_SCALE 50

int Fibonacci(const Fibonacci_in * fi, Fibonacci_out * fo, Fibonacci_atoms *)
{
	int r = (int)( rand()/((double)RAND_MAX) * FIB_WORK_SCALE );
	fib(r);
	gettimeofday(&(fo->cpu_t),NULL);
	fo->transaction_number = fi->transaction_number;
	fo->sleep_req = fi->sleep_req;
	copytime(fo->sleep_t,fi->sleep_t);
	copytime(fo->start_t,fi->start_t);
	return 0;
}

mtime_t subtract(const mtime_t & from, const mtime_t & by)
{
	mtime_t res;
	copytime(res,from);
	res.tv_sec -= by.tv_sec;
	res.tv_usec -= by.tv_usec;
	if(res.tv_usec < 0L) {
		res.tv_sec--;
		res.tv_usec += 1000000;
	}
	return res;
}

void prtime(mtime_t & t, const char * label)
{
	printf("  %s: %ld sec %ld usec\n", label, t.tv_sec, t.tv_usec);
}

int Done(const Done_in * di, Done_out *, Done_atoms *)
{
	printf("transaction %d:\n", di->transaction_number);
	mtime_t res = subtract(di->sleep_t,di->start_t);
	printf("  sleep req :        %ld usec\n", di->sleep_req);
	prtime(res,"sleeping  ");
	res = subtract(di->cpu_t  ,di->sleep_t);
	prtime(res,"processing");
	res = subtract(di->cpu_t  ,di->start_t);
	prtime(res,"total     ");
	printf("\n");
	return 0;
}

void handleTerminated(int signo)
{
	printf(" signal caught, %d transactions out\n", tn);
	exit(-1);
}

int init(int , char * argv[])
{
	signal(SIGTERM, handleTerminated);
	return 0;
}


