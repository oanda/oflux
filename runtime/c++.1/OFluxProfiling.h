#ifndef _OFLUX_PROFILING
#define _OFLUX_PROFILING

#include "OFluxLinkedList.h"
#include <time.h>
#include <sys/time.h>

#ifdef PROFILING
namespace oflux {

extern const char * _cxx_opts;


class TimerStats {
public:
        TimerStats()
                : _count(0)
                , _us(0.0)
		, _max(0.0)
                {}
        void add(long usec)
                {
                        _count++;
                        _us += (double) usec;
			_max = max(_max,usec);
                }
        int count() { return _count; }
        double avg_usec() { return (_count ? _us / _count : 0.0); }
	double max_usec() { return _max; }
private:
	inline double max(double a, double b) { return (a < b ? b : a); }
private:
        int    _count;
        double _us;
	double _max;
};

class TimerStart;

typedef Removable<TimerStart, InheritedNode<TimerStart> > TimerStartRemovable;
typedef LinkedListRemovable<TimerStart, TimerStartRemovable > TimerList;

class TimerStart : public TimerStartRemovable {
public:
	friend class TimerPause;
        TimerStart(TimerStats * ts)
                : _ts(ts)
		, _less(0)
                { gettimeofday(&_tp,NULL); }
        ~TimerStart()
                {
                        struct timeval tp2;
                        gettimeofday(&tp2,NULL);
                        long delta =
                                (tp2.tv_sec-_tp.tv_sec)*1000000
                                + (tp2.tv_usec-_tp.tv_usec)
				- _less
				;
                        _ts->add(delta);
                }
	void reset() 
		{
			_less = 0;
			gettimeofday(&_tp,NULL);
		}
protected:
	void subtract(long less_delta) // for pausing
		{	
			_less += less_delta;
		}
private:
        TimerStats *   _ts;
        struct timeval _tp;
	long           _less;
};


class TimerStartPausable : public TimerStart {
public:
	TimerStartPausable(TimerStats * ts, TimerList & list)
		: TimerStart(ts)
		, _list(list)
		{ _list.insert_front(this); }
	~TimerStartPausable() { _list.remove(this); }
	inline TimerList & list() { return _list; }
private:
	TimerList & _list;
};

// pause a list of timers
class TimerPause {
public:
	TimerPause(TimerList & tl)
		: _list(tl)
                { gettimeofday(&_tp,NULL); }
	~TimerPause()
		{
                        struct timeval tp2;
                        gettimeofday(&tp2,NULL);
                        long delta =
                                (tp2.tv_sec-_tp.tv_sec)*1000000
                                + (tp2.tv_usec-_tp.tv_usec);
			TimerStart * tsptr = _list.first()->content();
			while(tsptr) {
				tsptr->subtract(delta);
				tsptr = tsptr->next()->content();
			}
		}
private:
	TimerList &    _list;
        struct timeval _tp;
};

} // namespace
#endif // PROFILING


#endif // _OFLUX_PROFILING
