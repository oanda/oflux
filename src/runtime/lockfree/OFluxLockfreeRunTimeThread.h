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
#include <boost/shared_ptr.hpp>
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
		oflux_log_trace("[%d] steal  %s %p from thread [%d]\n"
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
		oflux_log_info("thread %d (pthread %lu) %s %s %s q_len:%ld q_alw:%ld %s\n"
			, _index
			, _tid
			, _running ? "running" : "       "
			, _request_stop ? "req-stop" : "        "
			, _asleep ? "asleep" : "      "
			, _queue.size()
			, _queue_allowance
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
		oflux_log_trace("[%d] popLocal %s %p\n"
			, self()
			, (ebptr.get() ? ebptr->flow_node()->getName() : "<null>")
			, ebptr.get());
		return ebptr;
	}
	inline void pushLocal(const EventBasePtr & ev)
	{
		oflux_log_trace("[%d] pushLocal %s %p\n"
			, self()
			, ev->flow_node()->getName()
			, ev.get());
		WSQElement * e = allocator.get(ev);
		_queue.pushBottom(e);
	}
	int handle(EventBasePtr & ev);
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
};

} // namespace lockfree
} // namespace oflux


#endif
