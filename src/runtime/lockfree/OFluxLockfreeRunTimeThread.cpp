#include "lockfree/OFluxLockfreeRunTimeThread.h"
#include "lockfree/OFluxLockfreeRunTime.h"
#include "OFluxWrappers.h"
#include "flow/OFluxFlowNode.h"
#include "lockfree/OFluxThreadNumber.h"
#include "event/OFluxEventBase.h"
#include "event/OFluxEventOperations.h"
#include "atomic/OFluxAtomicHolder.h"
#include "OFluxLogging.h"

namespace oflux {
namespace lockfree {

boost::shared_ptr<Allocator<RunTimeThread::WSQElement> >
RunTimeThread::allocator(new Allocator<RunTimeThread::WSQElement>(new MallocAllocatorImplementation<sizeof(RunTimeThread::WSQElement)>()));

RunTimeThread::RunTimeThread(RunTime & rt, int index, oflux_thread_t tid)
	: _next(NULL)
	, _rt(rt)
	, _index(index)
	, _running(false)
	, _request_stop(false)
	, _asleep(false)
	, _tid(tid)
	, _flow_node_working(NULL)
{
	oflux_mutex_init(&_lck);
	oflux_cond_init(&_cond);
}

RunTimeThread::~RunTimeThread()
{
	//oflux_mutex_destroy(&_lck);
	//oflux_cond_destroy(&_cond);
}

static void *
RunTimeThread_start_thread(void *pthis)
{
        RunTimeThread * rtt = static_cast<RunTimeThread*>(pthis);
	ThreadNumber::init(rtt->index());
        rtt->start();
        return NULL;
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
	oflux_log_debug("RunTimeThread::start() called -- thread index %d\n", index());
		// only this thread locks its own _lck
		// this is only needed for the _cond (signalling)
	SetTrue st(_running);
	int no_ev_iterations = 0;
	while(!_request_stop && !_rt.was_soft_killed()) {
		if(_rt.caught_soft_load_flow()) {
			oflux_log_debug("RunTimeThread::start() called -- reloading flow %d\n",index());
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
			int num_alive_threads = _rt.nonsleepers();
			int threads_to_wake = std::max( (num_new_evs>1 ? 1 : 0)
					, num_alive_threads*num_alive_threads - num_new_evs);
			oflux_log_debug("RunTimeThread::start() calling handle %d wt: %d\n",index(),threads_to_wake);
			_rt.wake_threads(threads_to_wake);
			// attempt to avoid a thundering herd here
		}
		_asleep = true;
		if(no_ev_iterations > NO_EV_CRITICAL && _rt.incr_sleepers()) {
			// have permission to sleep now
			oflux_log_debug("RunTimeThread::start() sleeping %d\n",index());
			oflux_cond_wait(&_cond, &_lck);
			_asleep = false;
			_rt.decr_sleepers();
			no_ev_iterations = 0;
		} else if(no_ev_iterations > NO_EV_CRITICAL*2
				&& _rt.all_asleep_except_me()) {
			_asleep = false;
			oflux_log_warn("RunTimeThread::start() exiting... seem to be out of events to run\n");
			_rt.soft_kill();
			break;
		}
		_asleep = false;
	}
}

int
RunTimeThread::handle(EventBasePtr & ev)
{
	_flow_node_working = ev->flow_node();
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
        // ------------ Release held atomics --------------
        // put the released events as priority on the queue
        std::vector<EventBasePtr> successor_events_released;
        ev->atomics().release(successor_events_released,ev);
        for(size_t i = 0; i < successor_events_released.size(); ++i) {
                EventBasePtr & succ_ev = successor_events_released[i];
                if(succ_ev->atomics().acquire_all_or_wait(succ_ev)) {
                        successor_events.push_back(successor_events_released[i]);
                }
        }
        for(size_t i = 0; i < successor_events.size(); ++i) {
		oflux_log_debug(" %u handle: %s (%d) succcessor %s pushed %d\n"
			, index()
			, _flow_node_working->getName()
			, i
			, successor_events[i]->flow_node()->getName()
			, return_code);
		pushLocal(successor_events[i]);
	}
	_flow_node_working = NULL;
	return successor_events.size();
}

} // namespace lockfree
} // namespace oflux
