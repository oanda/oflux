#ifndef OFLUX_LOCKFREE_RUNTIME
#define OFLUX_LOCKFREE_RUNTIME

/**
 * @file OFluxLockfreeRunTime.h
 * @author Mark Pichora
 *  Implementation of the lock-free runtime.  This runtime does not use
 * a central mutex (like the classic one), but rather relies on threads to
 * work independently on their own work stealing deques.
 */

#include "OFluxRunTimeAbstract.h"
#include "lockfree/OFluxLockfreeRunTimeThread.h"
#include "lockfree/OFluxThreadNumber.h"
#include "OFluxDoor.h"
#include "OFluxThreads.h"
#include <vector>

namespace oflux {
namespace flow {
 class Flow;
} // namespace flow
namespace lockfree {

class RunTimeThread;

typedef void (*deinitShimFnType) ();

class RunTime : public RunTimeAbstract {
public:
        static deinitShimFnType deinitShim;

	RunTime(const RunTimeConfiguration & rtc);
	virtual ~RunTime();

	virtual void start();
	virtual void soft_load_flow() { _soft_load_flow = true; }
	bool caught_soft_load_flow()
	{
		return _soft_load_flow && __sync_bool_compare_and_swap(&_soft_load_flow,true,false);
	}
	virtual int thread_count() { return _num_threads; }
	virtual RunTimeThreadAbstract * thread() { return _thread; }
	virtual void soft_kill();
	virtual void hard_kill() { soft_kill(); }
	bool was_soft_killed() { return _request_death; }
	virtual const RunTimeConfiguration & config() const { return _rtc; }
	virtual void log_snapshot();
	virtual void log_snapshot_guard(const char *);
	virtual void getPluginNames(std::vector<std::string> & result);
	virtual int atomics_style() const { return 2; }
	//
	bool incr_sleepers(); // false if that would put it at _num_threads
	void decr_sleepers();
	bool all_asleep_except_me() const { return _sleep_count+1 == _num_threads; }
	int nonsleepers() const { return _num_threads - _sleep_count; }
	virtual void submitEvents(const std::vector<EventBasePtr> & evs);
	void wake_threads(int num_to_wake)
	{ // rouse threads from their slumber
		if(_sleep_count == 0) return; // all awake already
		RunTimeThread * rtt = _threads;
		while(rtt && num_to_wake > 0) {
			if(rtt->asleep()) {
				rtt->wake();
				--num_to_wake;
			}
			rtt = rtt->_next;
		}
	}
	EventBasePtr steal_first_random()
	{ // do a steal sweep
		EventBasePtr ev;
		RunTimeThread * rtt = _threads;
		int start = rand() % _num_threads;
		int local_index = _tn.index;
		while(rtt) {
			if((rtt->index() >= start)
					&& rtt->index() != local_index) {
				ev = rtt->steal();
				if(get_EventBasePtr(ev)) {
					return ev;
				}
			}
			rtt = rtt->_next;
		}
		rtt = _threads;
		while(rtt && rtt->index() < start) {
			if(rtt->index() != local_index) {
				ev = rtt->steal();
				if(get_EventBasePtr(ev)) {
					break;
				}
			}
			rtt = rtt->_next;
		}
		return ev;
	}
	void load_flow(const char * filename = "", 
                   const char * pluginxmldir = "", 
                   const char * pluginlibdir = "",
                   void * initpluginparams = NULL);

	struct ActiveFlow {
		ActiveFlow(flow::Flow * f)
			: flow(f)
			, next(NULL)
		{}

		flow::Flow * flow;
		ActiveFlow * next; // older flows
	};
	void setupDoorsThread();
	RunTimeThread * doorsThread() { return _doors_thread; }
protected:
	void distribute_events(
		  RunTimeThread * rtt
		, std::vector<EventBasePtr> & events);
	virtual flow::Flow * flow() 
	{ return (_active_flow ? _active_flow->flow : NULL); }
private:
	const RunTimeConfiguration & _rtc_ref;
	RunTimeConfiguration _rtc;
	bool _running;
	bool _request_death;
	bool _soft_load_flow;
	int _num_threads;
	int _sleep_count;
	RunTimeThread * _threads;
	ActiveFlow * _active_flow;
	doors::ServerDoorsContainer _doors;
	RunTimeThread * _doors_thread;
public:
	static __thread RunTimeThread * _thread;
};

} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_RUNTIME
