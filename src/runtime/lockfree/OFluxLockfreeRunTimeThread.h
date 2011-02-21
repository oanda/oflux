#ifndef OFLUX_LF_RUNTIME_THREAD
#define OFLUX_LF_RUNTIME_THREAD

/**
 * @file OFluxLockfreeRunTimeThread.h
 * @author Mark Pichora
 *   Thread object used by the lock-free runtime.  
 */

#include "OFlux.h"
#include "OFluxAllocator.h"
#include "OFluxThreads.h"
#include "OFluxRunTimeThreadAbstract.h"
#include "lockfree/OFluxWorkStealingDeque.h"
#include "OFluxSharedPtr.h"
#include <signal.h>

#include "OFluxLogging.h"
#include "event/OFluxEventBase.h"
#include "flow/OFluxFlowNode.h"

namespace oflux {
namespace flow {
 class Node;
} //namespace flow
namespace lockfree {

class RunTime;

struct RunTimeThreadContext {
	enum SCategories 
		{ SC_high_guards = 0
		, SC_low_guards = SC_high_guards+1
		, SC_source = SC_low_guards+1
		, SC_no_guards = SC_source+1
		, SC_exec_gapped = SC_no_guards+1
		, SC_num_categories = SC_exec_gapped+1
		, SC_high_atomics_count = 3 // what is considered high
		, SC_low_atomics_count = 1  // what is considered low
		, SC_critical_execution_gap = 100
		};
	// this is used to avoid re-creating variables and vectors within the runtime loop
	// these are roughly in the order that they are used for one handle() iteration
	EventBasePtr ev;
	EventBase * evb; // _this_event
	flow::Node * flow_node_working;
	std::vector<EventBasePtr> successor_events;
	std::vector<EventBasePtr> successor_events_released;
	std::vector<EventBasePtr> successors_categorized[SC_num_categories];
};

class RunTimeThread : public ::oflux::RunTimeThreadAbstract {
public:
	friend class RunTime;

	struct WSQElement {
		WSQElement() {}
		WSQElement(EventBasePtr a_ev) : ev(a_ev)  {}
		~WSQElement();

		EventBasePtr ev;
	};
	typedef CircularWorkStealingDeque<WSQElement> WorkStealingDeque;

	static Allocator<WSQElement> allocator;

	RunTimeThread(RunTime & rt, int index, oflux_thread_t tid);
	~RunTimeThread();
	void start();
	virtual void submitEvents(const std::vector<EventBasePtr> &);
	virtual EventBase * thisEvent() const 
	{ return (_context ? _context->evb : NULL); }
	// a few functions just there for the abstract interface
	virtual bool is_detached() { return true; }
	virtual void set_detached(bool) {}
	virtual void wait_state(RTT_WaitState) {}
	virtual oflux_thread_t tid() { return _tid; }
	//
	inline EventBasePtr steal()
	{
		EventBasePtr ev;
		EventBase * evb = NULL;
		WSQElement * e = _queue.steal();
		if(e && e != &WorkStealingDeque::empty 
				&& e != &WorkStealingDeque::abort) {
			ev.swap(e->ev);
			evb = ev.get();
			allocator.put(e);
			evb->state = 3;
		}
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] steal  %s %p from thread [" PTHREAD_PRINTF_FORMAT "]\n"
			, oflux_self()
			, evb ? evb->flow_node()->getName() : "<null>"
			, evb
			, self());
		return ev;
	}
	int index() const { return _index; }
	void wake();
	bool die();
	bool asleep() const { return _asleep; }
	oflux_thread_t self() const { return _tid; }
	void log_snapshot()
	{
		flow::Node * fn = (_context ? _context->flow_node_working : NULL);
		const char * fn_name = (fn ? fn->getName() : "<null>");
		oflux_log_info("thread %d (pthread %lu) %s %s %s q_len:%ld q_alw:%ld slps:%lu e.run:%lu e.stl:%lu e.stl.at:%lu %s %p\n"
			, _index
			, _tid
			, _running ? "running" : "       "
			, _request_stop ? "req-stop" : "        "
			, _asleep ? "asleep" : "      "
			, _queue.size()
			, _queue_allowance
			, _stats.sleeps
			, _stats.events.run
			, _stats.events.stolen
			, _stats.events.attempts_to_steal
			, fn_name
			, thisEvent());
	}
protected:
	int create();
	RunTimeThread * _next;
private:
	inline EventBasePtr popLocal()
	{
		EventBasePtr ebptr;
		WSQElement * e = _queue.popBottom();
		if(e && e != &WorkStealingDeque::empty) {
			ebptr.swap(e->ev);
			allocator.put(e);
			ebptr->state = 2;
		}
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] popLocal %s %p\n"
			, self()
			, (ebptr.get() ? ebptr->flow_node()->getName() : "<null>")
			, ebptr.get());
		return ebptr;
	}
	inline void pushLocal(const EventBasePtr & ev)
	{
		ev->state = 1;
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] pushLocal %s %p\n"
			, self()
			, ev->flow_node()->getName()
			, ev.get());
		WSQElement * e = allocator.get(ev);
		_queue.pushBottom(e);
	}
	int handle(RunTimeThreadContext & context);
	inline bool critical() const { return _running && _queue_allowance<0; }
private:
	RunTime & _rt;
	int _index;
public:
	bool _running;
private:
	bool _request_stop;
	bool _asleep;
	WorkStealingDeque _queue;
	long _queue_allowance;
public:
	oflux_thread_t _tid;
private:
	oflux_mutex_t _lck; 
		// lock for the condition var (not really used as a lock)
	oflux_cond_t _cond;
		// conditional used for parking this thread
	RunTimeThreadContext * _context;
	struct Stats {
		Stats() : sleeps(0) {}
		struct Events {
			Events() : run(0), stolen(0), attempts_to_steal(0) {}
			unsigned long run;
			unsigned long stolen;
			unsigned long attempts_to_steal;
		} events;
		unsigned long sleeps;
	} _stats;
};

} // namespace lockfree
} // namespace oflux


#endif
