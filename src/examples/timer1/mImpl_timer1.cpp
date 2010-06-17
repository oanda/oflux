#include "OFluxGenerate_timer1.h"
#include "atomic/OFluxAtomicInit.h"
#include "TimerInterface.h"
#include <ctime>
#include <cstdio>

void handlesigalrm(int)
{
	signal(SIGALRM,handlesigalrm);
	// nothing to do
	// handler should be registered since Timer module uses it
}

void
init(int argc, char * argv[])
{
	OFLUX_TIMER_INIT(t);
	signal(SIGALRM,handlesigalrm);
}

class CustomTimerEvent : public Timer::EventBase {
public:
	CustomTimerEvent(size_t id)
		: Timer::EventBase(id)
	{}

};


template< typename Ga
	, const size_t offset > // each generator gets an id space
int
Generator(Ga *atoms)
{
	static size_t uniq_id = 1;
	sleep(1);
	time_t nowtime = time(NULL);
	enum { iterations = 10 };
	Timer::State * & state = atoms->state();
	Timer::Submitter submitter(*state);
	static size_t last_added = 0;
	for(size_t i = 0; i < iterations; ++i) {
		if(last_added-i>0 && (nowtime+i)%2 == 0) {
			submitter.cancel(last_added-i);
		}
	}
	for(size_t i = 0; i < iterations; ++i) {
		size_t new_id = (++uniq_id) * 2 + offset;
		Timer::EventBase * eb = new CustomTimerEvent(new_id);
		eb->expiry(nowtime+i);
		submitter.add(eb);
	}
	last_added = uniq_id;
	
	return 0;
}

int
Generator1(const Generator1_in *, Generator1_out *, Generator1_atoms * atoms)
{
	return Generator<Generator1_atoms,0>(atoms);
}

int
Generator2(const Generator2_in *, Generator2_out *, Generator2_atoms * atoms)
{
	return Generator<Generator2_atoms,1>(atoms);
}

template<typename In>
int
Reporter( const In * in, const char * funcname)
{
	time_t nowtime = time(NULL);
	printf("%ld %-17s received event id %-9u at %ld  %s\n"
		, nowtime
		, funcname
		, in->timer->id()
		, in->timer->expiry()
		, (nowtime > in->timer->expiry()
		  ? "*" // late marker
		  : ""));
	return 0;
}

int
ExpiredReporter(
	  const ExpiredReporter_in * in
	, ExpiredReporter_out *
	, ExpiredReporter_atoms *)
{ return Reporter(in, __FUNCTION__); }


int
CancelledReporter(
	  const CancelledReporter_in * in
	, CancelledReporter_out *
	, CancelledReporter_atoms *)
{ return Reporter(in, __FUNCTION__); }
