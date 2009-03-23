#ifndef _OFLUX_RUNTIME
#define _OFLUX_RUNTIME

/**
 * @file OFluxRunTime.h
 * @author Mark Pichora
 * Definitions for the OFlux run time and his threads
 */

#include "OFluxRunTimeBase.h"
#include "OFluxWrappers.h"
#include "OFluxQueue.h"
#include "OFluxLinkedList.h"
#include "boost/shared_ptr.hpp"

namespace oflux {
namespace runtime {
namespace classic { // the runtime we know and love

class RunTimeThread;
class RunTime;

typedef Removable<RunTimeThread, InheritedNode<RunTimeThread> > RunTimeThreadNode;
typedef LinkedListRemovable<RunTimeThread, RunTimeThreadNode> RunTimeThreadList;
/**
 * @class RunTime
 * @brief implementation of the core runtime (starts the initial thread)
 * Also has the functionality used by the shim to create / punt-to new
 * threads
 */
class RunTime : public RunTimeBase {
public:
	friend class RunTimeThread;
	friend class UnlockRunTime;

	RunTime(const RunTimeConfiguration & rtc);
        virtual ~RunTime();

        /**
         * @brief cede control to the runtime (make it go!)
         */
	virtual void start();
        /**
         * @brief load a particular flow
         */
	void load_flow(const char * filename = "", 
                   const char * pluginxmldir = "", 
                   const char * pluginlibdir = "",
                   void * initpluginparams = NULL);

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
	virtual void log_snapshot();
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
        virtual RunTimeThread * new_RunTimeThread(oflux_thread_t tid = 0);
        virtual void hard_kill();
protected:
	inline flow::Flow * flow() { return _active_flows.front(); }
	void remove(RunTimeThread * rtt);
protected:
	RunTimeThreadList   _thread_list;
	std::deque<flow::Flow *>  _active_flows;
	Queue               _queue;
	int                 _thread_count;
	int                 _detached_count;
};


/**
 * @class RunTimeThread
 * @brief thread object that runs the handle loop
 */
class RunTimeThread : public RunTimeThreadNode,
		public RunTimeThreadAbstract {
public:
	RunTimeThread(RunTime * rt, oflux_thread_t tid = 0)
		: _rt(rt)
                , _condition_context_switch(false)
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
	virtual void start();
	void handle(boost::shared_ptr<EventBase> & ev);
	virtual int execute_detached(boost::shared_ptr<EventBase> & ev,
                int & detached_count_to_increment);
	virtual bool is_detached() { return _detached; }
	virtual void set_detached(bool d) { _detached = d; }
	virtual oflux_thread_t tid() { return _tid; }
	void log_snapshot();
	void soft_die() { _request_death = true; }
	void hard_die() { _request_death = true; oflux_cancel(_tid); }
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
	inline flow::Node * working_flow_node() { return _flow_node_working; }
	virtual void wait_state(RTT_WaitState ws) { _wait_state = ws; }
protected:
        inline void enqueue_list(std::vector<boost::shared_ptr<EventBase> > & events) { _rt->_queue.push_list(events); }
protected:
	RunTime *      _rt;
        bool           _condition_context_switch;
	bool           _bootstrap;
	bool &         _system_running;
	bool           _thread_running;
	bool           _request_death;
	bool           _detached;
	RTT_WaitState  _wait_state;
	oflux_thread_t _tid;
	flow::Node *   _flow_node_working;
#ifdef PROFILING
	TimerList      _timer_list;
	TimerStartPausable * _oflux_timer;
#endif
};

} // namespace classic
} // namespace runtime
} // namespace oflux

#endif // _OFLUX_RUNTIME
