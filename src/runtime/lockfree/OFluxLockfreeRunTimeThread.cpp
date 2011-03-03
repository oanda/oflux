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
#include "lockfree/allocator/OFluxSMR.h"
#include "OFluxLogging.h"

#include "atomic/OFluxLFAtomic.h"

namespace ofluximpl {
    extern oflux::atomic::AtomicMapAbstract * IntPool_map_ptr;
}

namespace oflux {
namespace lockfree {

#ifdef SHARED_PTR_EVENTS
Allocator<RunTimeThread::WSQElement>
RunTimeThread::allocator; //(new allocator::MemoryPool<sizeof(RunTimeThread::WSQElement)>());
#endif // SHARED_PTR_EVENTS

RunTimeThread::RunTimeThread(RunTime & rt, int index, oflux_thread_t tid)
	: _next(NULL)
	, _rt(rt)
	, _index(index)
	, _running(false)
	, _request_stop(false)
	, _asleep(false)
	, _queue_allowance(0)
	, _tid(tid)
	, _context(NULL)
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
		EventBase * evb = get_EventBasePtr(ev);
		if(!evb) {
			break;
		} else {
			oflux_log_debug("~RunTimeThread discarding queued event %s %p\n"
				, evb->flow_node()->getName()
				, evb
				);
		}
	}
}

#ifdef SHARED_PTR_EVENTS
RunTimeThread::WSQElement::~WSQElement()
{}
/*{
	if(get_EventBasePtr(ev))  {
		oflux_log_debug("~WSQE %s %d\n"
			, ev->flow_node()->getName()
			, ev.use_count());
	}
}*/
#endif // SHARED_PTR_EVENTS

static void *
RunTimeThread_start_thread(void *pthis)
{
        RunTimeThread * rtt = static_cast<RunTimeThread*>(pthis);
	ThreadNumber::init(rtt->index());
	::oflux::lockfree::smr::DeferFree::init();
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
		oflux_log_trace("OFluxLockfreeRunTimeThread::die() sending tid "
			PTHREAD_PRINTF_FORMAT
			" (index %d) a SIGINT from " 
			PTHREAD_PRINTF_FORMAT
			" (%d)\n", _tid, index(), oflux_self(), _tn.index);
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
	_rt._thread = this; // thread local reference
	AutoLock al(&_lck); 
	oflux_log_trace("[" 
			PTHREAD_PRINTF_FORMAT
			"] RunTimeThread::start() called -- thread index %d\n"
			, oflux_self()
			, index());
		// only this thread locks its own _lck
		// this is only needed for the _cond (signalling)
	SetTrue st(_running);
	int no_ev_iterations = 0;
	assert(_tn.index == (size_t)_index);
	RunTimeThreadContext context;
	_context = &context;
	while(!_request_stop && !_rt.was_soft_killed()) {
		if(_rt.caught_soft_load_flow()) {
			oflux_log_trace("[" 
				PTHREAD_PRINTF_FORMAT
				"] RunTimeThread::start() called -- reloading flow %d\n"
				, oflux_self()
				, index());
			_rt.load_flow();
		}
		enum Q_Stealing {
			QS_Frequency = 100
		};
		
		if((_queue_allowance%QS_Frequency) ==0) {
			// skip pop local now and then 
			//   to contribute some stealing
			--_queue_allowance;
		} else {
			context.ev = mk_EventBaseSharedPtr(popLocal());
		}
		context.evb = context.ev.get();
		if(!context.evb) {
			++_stats.events.attempts_to_steal;
			context.ev = mk_EventBaseSharedPtr(_rt.steal_first_random());
			context.evb = context.ev.get();
			_stats.events.stolen += (context.evb ? 1 : 0);
		}
		if(!context.evb) {
			++no_ev_iterations;
#define SPIN_ITERATIONS 300
#define NO_EV_CRITICAL  100
			// spin for a while
			for(size_t i = 0; i < SPIN_ITERATIONS; ++i) {
				size_t isquared __attribute__((unused)) = i * i;
			}
		} else {
			no_ev_iterations = 0;
			int num_new_evs = handle(context);
			int num_alive_threads __attribute__((unused)) = _rt.nonsleepers();
			int threads_to_wake = num_new_evs;
			oflux_log_trace("[" 
				PTHREAD_PRINTF_FORMAT
				"] RunTimeThread::start() calling handle %d wt: %d\n"
				, oflux_self()
				, index()
				, threads_to_wake);
			_rt.wake_threads(threads_to_wake);
			// attempt to avoid a thundering herd here
		}
		_asleep = true;
		if(no_ev_iterations > NO_EV_CRITICAL && _rt.incr_sleepers()) {
			// have permission to sleep now
			++_stats.sleeps;
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
			, context.ev.use_count()
			, context.evb ? context.evb->flow_node()->getName() : "<null>"
			, context.evb && context.evb->get_predecessor() 
				? context.evb->get_predecessor().use_count()
				: 0
			, context.evb && context.evb->get_predecessor()
				? context.evb->get_predecessor()->flow_node()->getName()					: "<none>" );
		context.evb = NULL;
		context.ev.reset();
	}
}

void
RunTimeThread::wake()
{ 
	oflux_log_debug("RunTimeThread::wake() on %d\n",index());
	oflux_cond_signal(&_cond); 
}

int
RunTimeThread::handle(RunTimeThreadContext & context)
{
	context.flow_node_working = context.ev->flow_node();
	context.successor_events.clear();
	context.successor_events_released.clear();
	for(size_t ct= 0; ct < RunTimeThreadContext::SC_num_categories; ++ct) {
		context.successors_categorized[ct].clear();
	}
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] RunTimeThread::handle() on %s %p\n"
		, oflux_self()
		, context.flow_node_working->getName()
		, context.evb);
	// ---------------- Execution -------------------
	assert(context.ev->state != 5 && "detect double execution");
	int return_code = context.ev->execute();
	context.ev->state = 5;
	++_stats.events.run;
        // ----------- Successor processing -------------
        if(return_code) { // error encountered
                event::successors_on_error(
                          context.successor_events // output
                        , context.ev
                        , return_code);
        } else { // no error encountered
                event::successors_on_no_error(
                          context.successor_events // output
                        , context.ev);
        }
#ifdef OFLUX_DEEP_LOGGING
	for(size_t i = 0; i < context.successor_events.size(); ++i) {
		oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] successor of %s %p ---> %s %p\n"
			, oflux_self()
			, context.ev->flow_node()->getName()
			, context.evb
			, context.successor_events[i]->flow_node()->getName()
			, get_EventBasePtr(context.successor_events[i]));
	}
#endif // OFLUX_DEEP_LOGGING
        // ------------ Release held atomics --------------
        // put the released events as priority on the queue
#ifdef SHARED_PTR_EVENTS
#define LOCAL_EV context.ev // is EventBasePtr always
#else  // SHARED_PTR_EVENTS
#define LOCAL_EV context.evb
#endif // SHARED_PTR_EVENTS
        context.ev->atomics().release(context.successor_events_released,LOCAL_EV);
        for(size_t i = 0; i < context.successor_events_released.size(); ++i) {
                EventBasePtr & succ_ev = context.successor_events_released[i];
                if(succ_ev->atomics().acquire_all_or_wait(succ_ev)) {
                        context.successor_events.push_back(succ_ev);
                } else {
			oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] acquire_all_or_wait() failure for "
				"%s %p on guards acquisition"
				, oflux_self()
				, succ_ev->flow_node()->getName()
				, get_EventBasePtr(succ_ev));
		}
        }

	// ---------- Queue State and Control ---------
	// Queue is: 
	//   * cold when it has 0 to < Q_Hot_size elements
	//   * hot when it has Q_Hot_size to < Q_Critical_size elements
	//   * critical when it has >= Q_Critical_size

	enum QueueControlParams 
		{ Q_Critical_size = 32 // 1024
		, Q_Hot_size = 8 // 256
		};
	if(_queue_allowance<=0) {
		_queue_allowance = Q_Critical_size - _queue.size();
	} else {
		--_queue_allowance;
	}
	bool push_sources_first = (_queue_allowance < 0);
	bool push_all_guards_last = (_queue_allowance < Q_Critical_size-Q_Hot_size);

	// -------- Categorization of successors ---------
	//  This will order them for pushing based on:
	//  * queue allowance
	//  * whether the event is:
	//    + a source
	//    + how many atomics it is holding

	
        for(size_t i = 0; i < context.successor_events.size(); ++i) {
		// categorizing successors:
		EventBasePtr & ev_ptr = context.successor_events[i];
		int num_atomics_held = ev_ptr->atomics().number();
		oflux::flow::Node * fn = ev_ptr->flow_node();
		long long execution_gap = fn->instances() - fn->executions();
		if(execution_gap > RunTimeThreadContext::SC_critical_execution_gap) {
			context.successors_categorized[RunTimeThreadContext::SC_exec_gapped].push_back(ev_ptr);
		} else if(fn->getIsSource()) {
			context.successors_categorized[RunTimeThreadContext::SC_source].push_back(ev_ptr);
		} else if(num_atomics_held >= RunTimeThreadContext::SC_high_atomics_count) {
			context.successors_categorized[RunTimeThreadContext::SC_high_guards].push_back(ev_ptr);
		} else if(num_atomics_held >= RunTimeThreadContext::SC_low_atomics_count) {
			context.successors_categorized[RunTimeThreadContext::SC_low_guards].push_back(ev_ptr);
		} else {
			context.successors_categorized[RunTimeThreadContext::SC_no_guards].push_back(ev_ptr);
		}
	}
	size_t ind =0;
#define PUSH_EVENTS_FOR(X) \
	for(size_t i = 0; i < context.successors_categorized[X].size(); ++i) { \
		EventBasePtr & ev_ptr = context.successors_categorized[X][i]; \
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] %u handle: %s %p (%d) succcessor %s %p pushed %d\n" \
			, oflux_self() \
			, index() \
			, context.flow_node_working->getName() \
			, context.evb \
			, ind \
			, ev_ptr->flow_node()->getName() \
			, get_EventBasePtr(ev_ptr) \
			, return_code); \
		pushLocal(ev_ptr); \
		++ind; \
	}

	const char * queue_temp = "cold";
	if(push_sources_first) { // queue is critical
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_source);
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_no_guards);
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_low_guards);
		queue_temp = "crit";
	} else if(push_all_guards_last) { // queue is hot
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_no_guards);
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_source);
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_low_guards);
		queue_temp = "hot ";
	} else { // queue is cold
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_no_guards);
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_source);
		PUSH_EVENTS_FOR(RunTimeThreadContext::SC_low_guards);
	}
	PUSH_EVENTS_FOR(RunTimeThreadContext::SC_high_guards);
	PUSH_EVENTS_FOR(RunTimeThreadContext::SC_exec_gapped);
	oflux_log_debug("[" PTHREAD_PRINTF_FORMAT "] push evs eg:%u hg:%u lg:%u sr:%u ng:%u %s %d\n"
		, oflux_self()
		, context.successors_categorized[RunTimeThreadContext::SC_exec_gapped].size()
		, context.successors_categorized[RunTimeThreadContext::SC_high_guards].size()
		, context.successors_categorized[RunTimeThreadContext::SC_low_guards].size()
		, context.successors_categorized[RunTimeThreadContext::SC_source].size()
		, context.successors_categorized[RunTimeThreadContext::SC_no_guards].size()
		, queue_temp
		, _queue.size()
		);

	context.flow_node_working = NULL;
	return context.successor_events.size();
}

} // namespace lockfree
} // namespace oflux
