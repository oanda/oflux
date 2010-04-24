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

		EventBasePtr ev;
	};
	typedef CircularWorkStealingDeque<WSQElement> WorkStealingDeque;

	static boost::shared_ptr<Allocator<WSQElement> > allocator;

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
			allocator->put(e);
		}
		return ebptr;
	}
	int index() const { return _index; }
	void wake() { oflux_cond_signal(&_cond); }
	bool die()
	{
		_request_stop = true;
		if(asleep()) {
			wake();
		}
		oflux_mutex_unlock(&_lck);
		size_t retries = 4;
		int res = 0;
		while(oflux_self() != _tid && _running && retries && res == 0) {
			res = oflux_kill_int(_tid);
			if(_running) usleep(50000); // 50 ms rest
		}
		return !_running;
	}
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
			allocator->put(e);
		}
		return ebptr;
	}
	inline void pushLocal(EventBasePtr & ev)
	{
		WSQElement * e = allocator->get(ev);
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
