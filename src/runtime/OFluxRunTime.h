#ifndef _OFLUX_RUNTIME
#define _OFLUX_RUNTIME
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file OFluxRunTime.h
 * @author Mark Pichora
 * Definitions for the OFlux run time and his threads
 */

#include "OFluxRunTimeBase.h"
#include "OFluxWrappers.h"
#include "OFluxQueue.h"
#include "OFluxLinkedList.h"
#include "OFluxSharedPtr.h"
#include "OFluxDoor.h"

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
                   PluginSourceAbstract * pluginxmldir = 0, 
                   const char * pluginlibdir = "",
                   void * initpluginparams = NULL);

	/**
	 * @brief punt to a new thread
	 */
	virtual int wake_another_thread();

	virtual RunTimeThreadAbstractForShim * thread();

	virtual bool currently_detached();

	/**
	 * @brief thread local data (RunTimeThread object)
	 */
	static ThreadLocalDataKey<RunTimeThread>  thread_data_key;
	/**
	 * @brief outputs as much info the to the log as is available on
	 * the runtime state
	 */
	virtual void log_snapshot();
	virtual void log_snapshot_guard(const char * guardname);

	/**
	 * @brief Number of threads the runtime has
	 */
	virtual int thread_count() { return _thread_count; }

	void create_door_thread();

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
        /**
         * @brief obtain the list of plugin names (in loaded order)
         */
        virtual void getPluginNames(std::vector<std::string> & result);
	virtual flow::Flow * flow() 
	{ return _active_flows.size() ? _active_flows.front() : NULL; }
	virtual void submitEvents(const std::vector<EventBasePtr> &);
protected:
	inline flow::Flow * _flow() 
	{ return _active_flows.size() ? _active_flows.front() : NULL; }
	void remove(RunTimeThread * rtt);
protected:
	RunTimeThreadList   _thread_list;
	std::deque<flow::Flow *>  _active_flows;
	Queue               _queue;
	int                 _thread_count;
	int                 _detached_count;
	doors::ServerDoorsContainer _doors;
	RunTimeThread *     _door_thread;
};


/**
 * @class RunTimeThread
 * @brief thread object that runs the handle loop
 */
class RunTimeThread : public RunTimeThreadNode,
		public RunTimeThreadAbstractForShim {
public:
	RunTimeThread(RunTime * rt, oflux_thread_t tid = 0)
		: _rt(rt)
		, _this_event(NULL)
                , _condition_context_switch(false)
		, _bootstrap(true)
		, _system_running(rt->_running)
		, _thread_running(false)
		, _request_death(false)
		, _detached(false)
		, _wait_state(RTTWS_running)
		, _tid(tid)
		, _flow_node_working(NULL)
#ifdef PROFILING
		, _oflux_timer(NULL)
#endif
	{}
	virtual ~RunTimeThread() {}
	int create();
	static void * start_door(void *);
	virtual void start();
        void disconnect_from_runtime() { _rt->remove(this); }
	void handle(EventBaseSharedPtr & ev);
	virtual int execute_detached(EventBaseSharedPtr & ev,
                int & detached_count_to_increment);
	virtual bool is_detached() { return _detached; }
	virtual void set_detached(bool d) { _detached = d; }
	virtual oflux_thread_t tid() { return _tid; }
	void log_snapshot();
	void soft_die() { _request_death = true; }
	void hard_die() { _request_death = true; oflux_cancel(_tid); }
	bool running() { return _thread_running; }
	virtual void submitEvents(const std::vector<EventBasePtr> &);
	virtual EventBase * thisEvent() const { return _this_event; }
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
        inline void enqueue_list(std::vector<EventBasePtr > & events) { _rt->_queue.push_list(events); }
protected:
	RunTime *      _rt;
	EventBase *    _this_event;
        bool           _condition_context_switch;
	bool           _bootstrap;
	bool &         _system_running;
public:
	bool           _thread_running;
protected:
	bool           _request_death;
	bool           _detached;
	RTT_WaitState  _wait_state;
public:
	oflux_thread_t _tid;
protected:
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
