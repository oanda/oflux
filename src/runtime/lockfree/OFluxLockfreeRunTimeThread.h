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
	inline EventBasePtr steal()
	{
		EventBasePtr ebptr;
		WSQElement * e = _queue.steal();
		if(e && e != &WorkStealingDeque::empty 
				&& e != &WorkStealingDeque::abort) {
			ebptr.swap(e->ev);
			allocator.put(e);
		}
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] steal  %s %p from thread [" PTHREAD_PRINTF_FORMAT "]\n"
			, oflux_self()
			, ebptr.get() ? ebptr->flow_node()->getName() : "<null>"
			, ebptr.get()
			, self());
		return ebptr;
	}
	int index() const { return _index; }
	void wake();
	bool die();
	bool asleep() const { return _asleep; }
	oflux_thread_t self() const { return _tid; }
	void log_snapshot()
	{
		flow::Node * fn = _flow_node_working;
		const char * fn_name = (fn ? fn->getName() : "<null>");
		oflux_log_info("thread %d (pthread %lu) %s %s %s q_len:%ld q_alw:%ld slps:%lu e.run:%lu e.stl:%lu e.stl.at:%lu %s\n"
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
			, fn_name);
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
		}
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] popLocal %s %p\n"
			, self()
			, (ebptr.get() ? ebptr->flow_node()->getName() : "<null>")
			, ebptr.get());
		return ebptr;
	}
	inline void pushLocal(const EventBasePtr & ev)
	{
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] pushLocal %s %p\n"
			, self()
			, ev->flow_node()->getName()
			, ev.get());
		WSQElement * e = allocator.get(ev);
		_queue.pushBottom(e);
	}
	int handle(EventBasePtr & ev);
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
	flow::Node *_flow_node_working;
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
