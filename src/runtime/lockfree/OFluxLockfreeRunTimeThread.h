#ifndef OFLUX_LF_RUNTIME_THREAD
#define OFLUX_LF_RUNTIME_THREAD

#include "OFlux.h"
#include "OFluxAllocator.h"
#include "OFluxThreads.h"
#include "lockfree/OFluxWorkStealingDeque.h"
#include <boost/shared_ptr.hpp>
#include <signal.h>

namespace oflux {
namespace flow {
 class Node;
} //namespace flow
namespace lockfree {

class RunTime;

class RunTimeThread {
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
	inline EventBasePtr steal()
	{
		EventBasePtr ebptr;
		WSQElement * e = _queue.steal();
		if(e && e != &WorkStealingDeque::empty 
				&& e != &WorkStealingDeque::abort) {
			ebptr.swap(e->ev);
			allocator.put(e);
		}
		return ebptr;
	}
	int index() const { return _index; }
	void wake();
	bool die();
	bool asleep() const { return _asleep; }
	oflux_thread_t self() const { return _tid; }
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
		return ebptr;
	}
	inline void pushLocal(EventBasePtr & ev)
	{
		WSQElement * e = allocator.get(ev);
		_queue.pushBottom(e);
	}
	int handle(EventBasePtr & ev);
private:
	RunTime & _rt;
	int _index;
	bool _running;
	bool _request_stop;
	bool _asleep;
	WorkStealingDeque _queue;
	long _queue_allowance;
	oflux_thread_t _tid;
	oflux_mutex_t _lck; 
		// lock for the condition var (not really used as a lock)
	oflux_cond_t _cond;
		// conditional used for parking this thread
	flow::Node *_flow_node_working;
};

} // namespace lockfree
} // namespace oflux


#endif
