#ifndef _OFLUX_RUNTIME
#define _OFLUX_RUNTIME

/**
 * @file OFluxRunTime.h
 * @author Mark Pichora
 * Definitions for the OFlux run time and his threads
 */

#include "OFlux.h"
#include "OFluxRunTimeAbstract.h"
#include "OFluxWrappers.h"
#include "OFluxQueue.h"
#include "OFluxLinkedList.h"
#include "boost/shared_ptr.hpp"
#include <deque>

namespace oflux {

class FlowFunctionMaps;

class LoggingAbstract;

/**
 * @class RunTimeConfiguration
 * @brief struct for holding the run-time configuration of the run time.
 * size of the stack, thread pool size (ignored), flow XML file name and
 * the flow maps structure.
 */
struct RunTimeConfiguration {
	int stack_size;
	int initial_thread_pool_size;
	int max_thread_pool_size;
	int max_detached_threads;
	int min_waiting_thread_collect; // waiting in pool collector
	int thread_collection_sample_period;
	const char * flow_filename;
	FlowFunctionMaps * flow_maps;
        const char * plugin_xml_dir;    // plugin xml directory
        const char * plugin_lib_dir;    // plugin lib directory
};

class RunTimeThread;
class RunTime;

typedef void (*initShimFnType) (RunTime *);

class Flow;

typedef Removable<RunTimeThread, InheritedNode<RunTimeThread> > RunTimeThreadNode;
typedef LinkedListRemovable<RunTimeThread, RunTimeThreadNode> RunTimeThreadList;
/**
 * @class RunTime
 * @brief implementation of the core runtime (starts the initial thread)
 * Also has the functionality used by the shim to create / punt-to new
 * threads
 */
class RunTime : public RunTimeAbstract {
public:
	friend class RunTimeThread;
	friend class UnlockRunTime;

	RunTime(const RunTimeConfiguration & rtc);
	void start();
	void load_flow(const char * filename = "", const char * plugindir = "");
	void soft_load_flow() { _load_flow_next = true; }
	void soft_kill() { _running = false; }

	/**
	 * @brief punt to a new thread
	 */
	virtual int wake_another_thread();

	virtual RunTimeThreadAbstract * thread();

	/**
	 * @brief thread local data (RunTimeThread object)
	 */
	static ThreadLocalDataKey<RunTimeThread>  thread_data_key;
	/**
	 * @brief outputs as much info the to the log as is available on
	 * the runtime state
	 */
	void log_snapshot();
protected:
	/**
	 * @brief determine if we can detach the thread (runtime limits allow it)
	 */
	bool canDetachMore() const;
	/**
	 * @brief determine if we can create a new thread (runtime limits allow it)
	 */
	bool canThreadMore() const;
	int threadCollectionSamplePeriod() const
		{ return _rtc.thread_collection_sample_period; }
	void doThreadCollection();
protected:
	inline Flow * flow() { return _active_flows.front(); }
	void remove(RunTimeThread * rtt);
private:
	const RunTimeConfiguration & _rtc;
	RunTimeThreadList   _thread_list;
protected:
	Queue               _queue;
	std::deque<Flow *>  _active_flows;
	bool                _running;
	int                 _thread_count;
	int                 _detached_count;
	bool                _load_flow_next;
};

class FlowNode;

/**
 * @class RunTimeThread
 * @brief thread object that runs the handle loop
 */
class RunTimeThread : public RunTimeThreadNode,
		public RunTimeThreadAbstract {
public:
	RunTimeThread(RunTime * rt, oflux_thread_t tid = 0)
		: _rt(rt)
		, _bootstrap(true)
		, _system_running(rt->_running)
		, _thread_running(false)
		, _request_death(false)
		, _detached(false)
		, _wait_state(RTTWS_running)
		, _tid(tid)
		, _flow_node_working(NULL)
		{}
	virtual ~RunTimeThread() {}
	int create();
	void start();
	void handle(boost::shared_ptr<EventBase> & ev);
	virtual bool is_detached() { return _detached; }
	virtual void set_detached(bool d) { _detached = d; }
	virtual oflux_thread_t tid() { return _tid; }
	void log_snapshot();
	void soft_die() { _request_death = true; }
	bool running() { return _thread_running; }
	inline void wait_to_run() 
		{
			_wait_state = RTTWS_wtr;
			_rt->wait_to_run();
			_wait_state = RTTWS_running;
		}
	inline void wait_in_pool() 
		{
			_wait_state = RTTWS_wip;
			_rt->wait_in_pool();
			_wait_state = RTTWS_running;
		}
#ifdef PROFILING
	virtual TimerList & timer_list() { return _timer_list; }
	virtual TimerStartPausable * oflux_timer() { return _oflux_timer; }
#endif
	inline FlowNode * working_flow_node() { return _flow_node_working; }
	virtual void wait_state(RTT_WaitState ws) { _wait_state = ws; }
private:
	RunTime *      _rt;
	bool           _bootstrap;
	bool &         _system_running;
	bool           _thread_running;
	bool           _request_death;
	bool           _detached;
	RTT_WaitState  _wait_state;
	oflux_thread_t _tid;
	FlowNode *     _flow_node_working;
#ifdef PROFILING
	TimerList      _timer_list;
	TimerStartPausable * _oflux_timer;
#endif
};


}; // namespace

#endif // _OFLUX_RUNTIME
