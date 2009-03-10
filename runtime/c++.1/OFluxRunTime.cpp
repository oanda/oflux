#include "OFluxRunTime.h"
#include "OFluxFlow.h"
#include "OFluxXML.h"
#include "OFluxEvent.h"
#include "OFluxAtomic.h"
#include "OFluxAtomicHolder.h"
#include "OFluxAcquireGuards.h"
#include "OFluxLogging.h"
#include "OFluxProfiling.h"
#include <unistd.h>
#include <dlfcn.h>


namespace oflux {
namespace runtime {
namespace classic {

//Flow * RunTime::__no_flow_is_some_flow = new Flow();

void _thread_local_destructor(void *t)
{
	RunTimeThread * rtt = static_cast<RunTimeThread *>(t);
	if(t) {
		RunTime::thread_data_key.set(NULL);
		oflux_log_info("runtime thread memory reclaimed for tid %d\n", rtt->tid());
		delete rtt; // no idea if this is safe
	}
}

ThreadLocalDataKey<RunTimeThread> RunTime::thread_data_key(_thread_local_destructor);

RunTime::RunTime(const RunTimeConfiguration & rtc)
	: RunTimeBase(rtc)
	//, _flow(__no_flow_is_some_flow)
	, _thread_count(0) // will count the news
	, _detached_count(0) 
{
	load_flow();
	// init the shim
	if(!RunTimeBase::initShim) {
                RunTimeBase::initShim = 
                        (initShimFnType)dlsym (RTLD_NEXT, "initShim");
        }
	if (RunTimeBase::initShim == NULL){
		oflux_log_error("ERROR no SHIM file found... exiting\n%s\n",dlerror());
		exit(0);
	}
	((RunTimeBase::initShim)(this));
}

RunTime::~RunTime()
{
        {
                Queue::Element e;
                while(_queue.pop(e)) {} // empty the event queue
        }
        while(_active_flows.size() > 0) {
                flow::Flow * back = _active_flows.back();
                _active_flows.pop_back();
                delete back;
        }
}

void RunTime::load_flow(const char * flname, const char * pluginxmldir, const char * pluginlibdir, void * initpluginparams)
{
        oflux_log_info("RunTime::load_flow() called\n");
	if(*flname == '\0') {
		flname = _rtc.flow_filename;
	}
	if(*pluginxmldir == '\0') {
		pluginxmldir = _rtc.plugin_xml_dir;
	}
	if(*pluginlibdir == '\0') {
		pluginlibdir = _rtc.plugin_lib_dir;
	}
        if(initpluginparams == NULL) {
                initpluginparams = _rtc.init_plugin_params;
        }
	// read XML file
        flow::Flow * flow = xml::read(flname, _rtc.flow_maps, pluginxmldir, pluginlibdir, initpluginparams);
        flow->assignMagicNumbers(); // for guard ordering
	// push the sources (first time)
	std::vector<flow::Node *> & sources = flow->sources();
	for(int i = 0; i < (int) sources.size(); i++) {
		flow::Node * fn = sources[i];
                oflux_log_info("load_flow pushing %s\n",fn->getName());
		CreateNodeFn createfn = fn->getCreateFn();
		boost::shared_ptr<EventBase> ev = (*createfn)(EventBase::no_event,NULL,fn);
		_queue.push(ev);
	}
	while(_active_flows.size() > 0) {
		flow::Flow * back = _active_flows.back();
		if(back->sources().size() > 0) {
			back->turn_off_sources();
		}
		if(back->has_instances()) {
			break;
		} 
		delete back;
		_active_flows.pop_back();
	};
	_active_flows.push_front(flow);
};


void RunTime::start()
{
	_running = true;
	RunTimeThread * rtt = new_RunTimeThread(oflux_self());
	_thread_list.insert_front(rtt);
	_thread_count++;
	RunTime::thread_data_key.set(rtt);
	rtt->start();
        while(_thread_count> 0) {
                AutoLock al(&_manager_lock);
        }
        remove(rtt);
        delete rtt;
}

void RunTime::remove(RunTimeThread * rtt)
{
	_thread_count--;
	_thread_list.remove(rtt);
}

static void log_snapshot_thread(RunTimeThread * rtt)
{
	rtt->log_snapshot();
}

void RunTime::log_snapshot()
{
	oflux_log_info("Runtime snapshot at %d on pid %d (%s)\n", fast_time(NULL), getpid(), (_running ? "running" : "not running"));
	oflux_log_info("Event Queue contents:\n");
	_queue.log_snapshot();
	oflux_log_info("Thread(s) state:\n");
	_thread_list.iter(log_snapshot_thread);
	oflux_log_info("Thread count: %d (%s %d)\n", _thread_count,
		(_rtc.max_thread_pool_size ? "maximum " : "no limit/"),
		_rtc.max_thread_pool_size);
	oflux_log_info("Detached Event count: %d (%s %d)\n", _detached_count,
		(_rtc.max_detached_threads ? "maximum " : "no limit/"),
		_rtc.max_detached_threads);
	oflux_log_info("Waiting to run count: %d\n", _waiting_to_run.count());
	oflux_log_info("Waiting in pool: %d\n", _waiting_in_pool.count());
#ifdef THREAD_COLLECTION
	char watermark_str[400];
	_waiting_in_pool.counter_implementation().log_snapshot(watermark_str,400);
	oflux_log_info("Waiting in pool watermarks: %s\n", watermark_str);
#endif
	oflux_log_info("Active Thread TID: %d\n", thread()->tid());
	oflux_log_info("Flow Summary:\n");
	flow()->log_snapshot();
}

RunTimeThreadAbstract * RunTime::thread()
{
	RunTimeThread * rtt = thread_data_key.get();
	assert(rtt);
	return rtt;
}

bool RunTime::canThreadMore() const
{
	return _rtc.max_thread_pool_size == 0
		|| _thread_count < _rtc.max_thread_pool_size;
}

bool RunTime::canDetachMore() const
{
	return _rtc.max_detached_threads == 0
		|| _detached_count < _rtc.max_detached_threads;
}

RunTimeThread * RunTime::new_RunTimeThread(oflux_thread_t tid)
{
        return new RunTimeThread(this,tid);
}

int RunTime::wake_another_thread()
{
	if(_running && _waiting_in_pool.count() == 0 && canThreadMore()) {
		RunTimeThread * rtt = new_RunTimeThread();
		_thread_count++;
		_thread_list.insert_front(rtt);
		return rtt->create();
	} else {
		if(_waiting_to_run.count() > 0) {
			_waiting_to_run.signal();
		} else if(_waiting_in_pool.count() > 0) {
			_waiting_in_pool.signal();
		} else { // if nothing available in conditional queues wait to run
			_waiting_to_run.signal();
		}
	}
	return 0;
}

static void * RunTimeThread_start_thread(void *pthis)
{
	RunTimeThread * rtt = static_cast<RunTimeThread*> (pthis);
	RunTime::thread_data_key.set(rtt);
	rtt->start();
	return NULL;
}

int RunTimeThread::create()
{
	return oflux_create_thread(
		  _rt->_rtc.stack_size // stack size
		, RunTimeThread_start_thread // run function
		, this // data argument to run function
		, &_tid); // where to put the thread id
}

void RunTime::doThreadCollection() 
{
#ifdef THREAD_COLLECTION
	WatermarkedCounter & watermarked_counter = _waiting_in_pool.counter_implementation();
	if(_rtc.min_waiting_thread_collect == 0) {
		watermarked_counter.next_sample();
		return;
	}
	int collect_idle = watermarked_counter.min() - _rtc.min_waiting_thread_collect;
	if(collect_idle > 0 ) {
		RunTimeThreadNode * rtt = _thread_list.first();
		oflux_log_info("runtime shedding %d threads\n", 
			(collect_idle > 0 ? collect_idle : 0));
		while(collect_idle > 0 && rtt != NULL) {
			rtt->content()->soft_die();
			rtt = rtt->next();
			collect_idle--;
		}
	}
	watermarked_counter.next_sample();
#endif // THREAD_COLLECTION
}

void RunTimeThread::start()
{
	SetTrue keep_true_during_lifetime(_thread_running);
	boost::shared_ptr<EventBase> ev;
	AutoLock al(&(_rt->_manager_lock));

	if(!_bootstrap) {
		wait_in_pool();
	}

	while(_system_running && !_request_death) {
		if(_condition_context_switch 
                                && _rt->_waiting_to_run.count() > 0 ) {
			AutoUnLock ual(&(_rt->_manager_lock));
		}
		if(_rt->_load_flow_next) {
			_rt->_load_flow_next = false;
			_rt->load_flow();
		}

#ifdef THREAD_COLLECTION
		static int thread_collection_sample_counter = 0;
#define THREAD_COLLECTION_PERIOD 6000
		thread_collection_sample_counter++;
		if(thread_collection_sample_counter 
				> _rt->threadCollectionSamplePeriod()) {
			thread_collection_sample_counter = 0;
			_rt->doThreadCollection();
			
	
		}
#endif // THREAD_COLLECTION

		if(_rt->_waiting_to_run.count() > 0) {
			_rt->_waiting_to_run.signal();
			wait_in_pool();
		}
		if(_rt->_queue.pop(ev)) {
			handle(ev);
		} else { // queue is empty - strange case
			_rt->_waiting_to_run.signal();
			wait_in_pool();
		}
	}
	_rt->remove(this);
	oflux_log_info("runtime thread %d is exiting\n", _tid);
	if(_rt->_thread_count > 0) {
                _rt->wake_another_thread();
        }
}

void RunTimeThread::log_snapshot()
{
	static const char * _wait_state_string[] =
		{ "running (app code)"
		, "waiting to run (rt)"
		, "waiting in pool (rt)"
		, "blocking call (shim)"
		, "waiting to run (shim)"
		, NULL
		};
	const char * working_on = 
		(_flow_node_working 
		? _flow_node_working->getName()
		: "<none>");
	oflux_log_info("%d (%c%c%c %s) job:%s\n",
		_tid,
		(_thread_running ? 'r' : '-'),
		(_detached ? 'd' : '-'),
		(_bootstrap ? 'b' : '-'),
		_wait_state_string[_wait_state],
		working_on);
}

int RunTimeThread::execute_detached(boost::shared_ptr<EventBase> & ev, 
        int & detached_count_to_increment)
{
        SetTrue st(_detached);
        Increment incr(detached_count_to_increment,_tid);
        _rt->wake_another_thread();
#ifdef PROFILING
        TimerPause oflux_tp(_timer_list);
#endif
        int return_code;
        {
                UnlockRunTime urt(_rt);
                return_code = ev->execute();
                incr.release(_tid);
        }
        return return_code;
}

void RunTimeThread::handle(boost::shared_ptr<EventBase> & ev)
{
	_flow_node_working = ev->flow_node();
	// ------------------ Guards --------------------
	if(AcquireGuards::doit(ev) == AcquireGuards::AGR_Success) {
	// ---------------- Execution -------------------
		int return_code;
		{ 
#ifdef PROFILING
			TimerStart real_timing_execution(ev->flow_node()->real_timer_stats());
			TimerStartPausable oflux_timing_execution(ev->flow_node()->oflux_timer_stats(), _timer_list);
			_oflux_timer = & oflux_timing_execution;
#endif
			if( ev->flow_node()->getIsDetached() && _rt->canDetachMore()) {
                                return_code = execute_detached(ev,_rt->_detached_count);
				wait_to_run();
			} else {
				return_code = ev->execute();
			}
#ifdef PROFILING
			_oflux_timer = NULL;
#endif
		}
	// ----------- Successor processing -------------
		std::vector<boost::shared_ptr<EventBase> > successor_events;
		std::vector<boost::shared_ptr<EventBase> > successor_events_priority;
		std::vector<boost::shared_ptr<EventBase> > successor_events_released;
		if(return_code) { // error encountered
			std::vector<flow::Case *> fsuccessors;
			void * ev_output = ev->output_type().next();
			ev->flow_node()->get_successors(fsuccessors, 
					ev_output, return_code);
			for(int i = 0; i < (int) fsuccessors.size(); i++) {
				flow::Node * fn = fsuccessors[i]->targetNode();
				flow::IOConverter * iocon = fsuccessors[i]->ioConverter();
				CreateNodeFn createfn = fn->getCreateFn();
				boost::shared_ptr<EventBase> ev_succ = 
					(*createfn)(ev->get_predecessor(),iocon->convert(ev->input_type()),fn);
				ev_succ->error_code(return_code);
				if(AcquireGuards::doit(ev_succ,ev->atomics()) == AcquireGuards::AGR_Success) {
					successor_events_priority.push_back(ev_succ);
				}
			}
		} else { // no error encountered
			std::vector<flow::Case *> fsuccessors;
			OutputWalker ev_ow = ev->output_type();
			void * ev_output = NULL;
			bool saw_source = false;
			while((ev_output = ev_ow.next()) != NULL) {
				fsuccessors.clear();
				ev->flow_node()->get_successors(fsuccessors, 
						ev_output, return_code);
				for(int i = 0; i < (int) fsuccessors.size(); i++) {
					flow::Node * fn = fsuccessors[i]->targetNode();
					flow::IOConverter * iocon = fsuccessors[i]->ioConverter();
					bool is_source = fn->getIsSource();
					if(is_source && saw_source) {
						continue;
						// only one source allowed
						// even with splaying
					}
					saw_source = saw_source || is_source;
					CreateNodeFn createfn = fn->getCreateFn();
					boost::shared_ptr<EventBase> ev_succ = 
						( is_source
						? (*createfn)(EventBase::no_event,NULL,fn)
						: (*createfn)(ev,iocon->convert(ev_output),fn)
						);
					ev_succ->error_code(return_code);
					if(AcquireGuards::doit(ev_succ,is_source
                                                ? AcquireGuards::empty_ah
                                                : ev->atomics()) == AcquireGuards::AGR_Success) {
						successor_events_priority.push_back(ev_succ);
					}
				}
			}
		}
	// ------------ Release held atomics --------------
		// put the released events as priority on the queue
		ev->atomics().release(successor_events_released);
		for(int i = 0; i < (int)successor_events_released.size(); i++) {
			if(AcquireGuards::doit(successor_events_released[i]) == AcquireGuards::AGR_Success) {
				successor_events_priority.push_back(successor_events_released[i]);
			}
		}

		enqueue_list(successor_events_priority); // no priority
		enqueue_list(successor_events);
	}
	_flow_node_working = NULL;
}


} //namespace classic
} //namespace runtime

RunTimeBase * _create_classic_runtime(const RunTimeConfiguration &rtc)
{
        return new runtime::classic::RunTime(rtc);
}

} //namespace oflux
