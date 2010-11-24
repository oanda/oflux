#include <inttypes.h>
#include "lockfree/OFluxLockfreeRunTimeThread.h"
#include "lockfree/OFluxLockfreeRunTime.h"
#include "OFluxWrappers.h"
#include "flow/OFluxFlowNode.h"
#include "lockfree/OFluxThreadNumber.h"
#include "event/OFluxEventBase.h"
#include "event/OFluxEventOperations.h"
#include "atomic/OFluxAtomicHolder.h"
#include "lockfree/allocator/OFluxLFMemoryPool.h"
#include "OFluxLogging.h"

#include "atomic/OFluxLFAtomic.h"

namespace ofluximpl {
    extern oflux::atomic::AtomicMapAbstract * IntPool_map_ptr;
}

namespace oflux {
namespace lockfree {

Allocator<RunTimeThread::WSQElement>
RunTimeThread::allocator; //(new allocator::MemoryPool<sizeof(RunTimeThread::WSQElement)>());

RunTimeThread::RunTimeThread(RunTime & rt, int index, oflux_thread_t tid)
	: _next(NULL)
	, _rt(rt)
	, _index(index)
	, _running(false)
	, _request_stop(false)
	, _asleep(false)
	, _queue_allowance(0)
	, _tid(tid)
	, _flow_node_working(NULL)
{
	oflux_mutex_init(&_lck);
	oflux_cond_init(&_cond);
}

RunTimeThread::~RunTimeThread()
{
	oflux_mutex_destroy(&_lck);
	oflux_cond_destroy(&_cond);
	while(_queue.size()) {
		EventBasePtr ev = popLocal();
		if(!ev.get()) {
			break;
		} else {
			oflux_log_debug("~RunTimeThread discarding queued event %s %p\n"
				, ev->flow_node()->getName()
				, ev.get()
				);
		}
	}
}

RunTimeThread::WSQElement::~WSQElement()
{}
/*{
	if(ev.get())  {
		oflux_log_debug("~WSQE %s %d\n"
			, ev->flow_node()->getName()
			, ev.use_count());
	}
}*/

static void *
RunTimeThread_start_thread(void *pthis)
{
        RunTimeThread * rtt = static_cast<RunTimeThread*>(pthis);
	ThreadNumber::init(rtt->index());
        rtt->start();
	oflux_log_trace("thread index %d finished\n",rtt->index());
        return NULL;
}

void
RunTimeThread::submitEvents(const std::vector<EventBasePtr> & evs)
{
	std::vector<EventBasePtr>::const_iterator itr = evs.begin();
	while(itr != evs.end()) {
		pushLocal(*itr);
		++itr;
	}
	_rt.wake_threads(1); // uhm....
}

extern bool __ignore_sig_int;

bool 
RunTimeThread::die()
{
	_request_stop = true;
	if(asleep()) {
		wake();
	}
	oflux_mutex_unlock(&_lck);
	size_t retries = 4;
	int res = 0;
	__ignore_sig_int = true;
	while(oflux_self() != _tid && _running && retries && res == 0) {
		oflux_log_trace("OFluxLockfreeRunTimeThread::die() sending tid %d (index %d) a SIGINT from %d (%d)\n", _tid, index(), pthread_self(), _tn.index);
		res = oflux_kill_int(_tid);
		if(_running) usleep(50000); // 50 ms rest
	}
	__ignore_sig_int = false;
	return !_running;
}

int
RunTimeThread::create()
{
        return oflux_create_thread(
                  _rt.config().stack_size // stack size
                , RunTimeThread_start_thread // run function
                , this // data argument to run function
                , &_tid); // where to put the thread id
}


void
RunTimeThread::start()
{
	AutoLock al(&_lck); 
	oflux_log_trace("[%d] RunTimeThread::start() called -- thread index %d\n"
			, oflux_self()
			, index());
		// only this thread locks its own _lck
		// this is only needed for the _cond (signalling)
	SetTrue st(_running);
	int no_ev_iterations = 0;
	while(!_request_stop && !_rt.was_soft_killed()) {
		if(_rt.caught_soft_load_flow()) {
			oflux_log_trace("[%d] RunTimeThread::start() called -- reloading flow %d\n"
				, oflux_self()
				, index());
			_rt.load_flow();
		}
		EventBasePtr ev = popLocal();
		if(!ev.get()) {
			ev = _rt.steal_first_random();
		}
		if(!ev.get()) {
			++no_ev_iterations;
#define SPIN_ITERATIONS 1000
#define NO_EV_CRITICAL  100
			// spin for a while
			for(size_t i = 0; i < SPIN_ITERATIONS; ++i) {
				size_t isquared __attribute__((unused)) = i * i;
			}
		} else {
			no_ev_iterations = 0;
			int num_new_evs = handle(ev);
			int num_alive_threads __attribute__((unused)) = _rt.nonsleepers();
			int threads_to_wake = num_new_evs;
			oflux_log_trace("[%d] RunTimeThread::start() calling handle %d wt: %d\n"
				, oflux_self()
				, index()
				, threads_to_wake);
			_rt.wake_threads(threads_to_wake);
			// attempt to avoid a thundering herd here
		}
		_asleep = true;
		if(no_ev_iterations > NO_EV_CRITICAL && _rt.incr_sleepers()) {
			// have permission to sleep now
			oflux_log_trace("RunTimeThread::start() sleeping %d\n",index());
			oflux_cond_wait(&_cond, &_lck);
			_asleep = false;
			oflux_log_trace("RunTimeThread::start() woke up  %d\n",index());
			_rt.decr_sleepers();
			no_ev_iterations = 0;
		} else if(no_ev_iterations > NO_EV_CRITICAL*2
				&& _rt.all_asleep_except_me()) {

			//oflux::lockfree::atomic::AtomicPool::dump(ofluximpl::IntPool_map_ptr);
			if(_rt.doorsThread()) {
				oflux_log_trace("RunTimeThread::start() there is a doors thread\n");
				oflux_log_trace("RunTimeThread::start() sleeping %d\n",index());
				oflux_cond_wait(&_cond, &_lck);
				_asleep = false;
				oflux_log_trace("RunTimeThread::start() woke up  %d\n",index());
			} else {
				_asleep = false;
				oflux_log_warn("RunTimeThread::start() exiting... seem to be out of events to run\n");
				_rt.soft_kill();
				break;
			}
		}
		_asleep = false;
		oflux_log_trace2("RunTimeThread::start (about to reset) "
			"ev %d %s ev.pred %d %s\n"
			, ev.use_count()
			, ev.get() ? ev->flow_node()->getName() : "<null>"
			, ev.get() && ev->get_predecessor() 
				? ev->get_predecessor().use_count()
				: 0
			, ev.get() && ev->get_predecessor()
				? ev->get_predecessor()->flow_node()->getName()					: "<none>" );
		ev.reset();
	}
}

void
RunTimeThread::wake()
{ 
	oflux_log_debug("RunTimeThread::wake() on %d\n",index());
	oflux_cond_signal(&_cond); 
}

int
RunTimeThread::handle(EventBasePtr & ev)
{
	_flow_node_working = ev->flow_node();
	oflux_log_trace("[%d] RunTimeThread::handle() on %s %p\n"
		, oflux_self()
		, _flow_node_working->getName()
		, ev.get());
	// ---------------- Execution -------------------
	int return_code = ev->execute();
        // ----------- Successor processing -------------
        std::vector<EventBasePtr> successor_events;
        if(return_code) { // error encountered
                event::successors_on_error(
                          successor_events // output
                        , ev
                        , return_code);
        } else { // no error encountered
                event::successors_on_no_error(
                          successor_events // output
                        , ev);
        }
#ifdef OFLUX_DEEP_LOGGING
	for(size_t i = 0; i < successor_events.size(); ++i) {
		oflux_log_trace2("[%d] successor of %s %p ---> %s %p\n"
			, oflux_self()
			, ev->flow_node()->getName()
			, ev.get()
			, successor_events[i]->flow_node()->getName()
			, successor_events[i].get());
	}
#endif // OFLUX_DEEP_LOGGING
        // ------------ Release held atomics --------------
        // put the released events as priority on the queue
        std::vector<EventBasePtr> successor_events_released;
        ev->atomics().release(successor_events_released,ev);
        for(size_t i = 0; i < successor_events_released.size(); ++i) {
                EventBasePtr & succ_ev = successor_events_released[i];
                if(succ_ev->atomics().acquire_all_or_wait(succ_ev)) {
                        successor_events.push_back(successor_events_released[i]);
                } else {
			oflux_log_trace2("[%d] acquire_all_or_wait() failure for "
				"%s %p on guards acquisition"
				, oflux_self()
				, succ_ev->flow_node()->getName()
				, succ_ev.get());
		}
        }

#define CRITICAL_Q_SIZE 1024
	if(_queue_allowance<=0) {
		_queue_allowance = CRITICAL_Q_SIZE - _queue.size();
	} else {
		--_queue_allowance;
	}
	bool push_sources_first = (_queue_allowance < 0);
	
	size_t ind =0;
        for(size_t i = 0; i < successor_events.size(); ++i) {
		if(push_sources_first == successor_events[i]->flow_node()->getIsSource()) {
			oflux_log_trace("[%d] %u handle: %s %p (%d) succcessor %s %p pushed %d\n"
				, oflux_self()
				, index()
				, _flow_node_working->getName()
				, ev.get()
				, ind
				, successor_events[i]->flow_node()->getName()
				, successor_events[i].get()
				, return_code);
			pushLocal(successor_events[i]);
			++ind;
		}
	}
        for(size_t i = 0; i < successor_events.size(); ++i) {
		if(push_sources_first != successor_events[i]->flow_node()->getIsSource()) {
			oflux_log_trace("[%d] %u handle: %s %p (%d) succcessor %s %p pushed %d\n"
				, oflux_self()
				, index()
				, _flow_node_working->getName()
				, ev.get()
				, ind
				, successor_events[i]->flow_node()->getName()
				, successor_events[i].get()
				, return_code);
			pushLocal(successor_events[i]);
			++ind;
		}
	}
	_flow_node_working = NULL;
	return successor_events.size();
}

} // namespace lockfree
} // namespace oflux
