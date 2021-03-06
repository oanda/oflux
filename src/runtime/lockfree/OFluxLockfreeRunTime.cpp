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
#include "lockfree/OFluxLockfreeRunTime.h"
#include "lockfree/OFluxThreadNumber.h"
#include "lockfree/allocator/OFluxSMR.h"
#include "event/OFluxEventOperations.h"
#include "flow/OFluxFlow.h"
#include "xml/OFluxXML.h"
#include "OFluxLogging.h"
#include "OFluxWrappers.h"
#include <dlfcn.h>
#include <signal.h>
#include <cstring>

namespace oflux {

RunTimeAbstract * 
_create_lockfree_runtime(const RunTimeConfiguration & rtc)
{
	return new lockfree::RunTime(rtc);
}


namespace lockfree {

deinitShimFnType RunTime::deinitShim = NULL;


RunTime::RunTime(const RunTimeConfiguration &rtc)
	: _rtc_ref(rtc)
	, _rtc(rtc)
	, _running(false)
	, _request_death(false)
	, _soft_load_flow(false)
	, _sleep_count(0)
	, _threads(NULL)
	, _active_flow(NULL)
	, _doors(this)
	, _doors_thread(NULL)
{
	_thread = NULL;
	oflux_log_info("oflux::lockfree::RunTime initializing\n");
	ThreadNumber::init(0); // count yourself in
	::oflux::lockfree::smr::DeferFree::init();
	if(rtc.initAtomicMapsF) {
		// create the AtomicMaps (guards)
		(*(rtc.initAtomicMapsF))(atomics_style());
	}
	if(!RunTime::deinitShim) {
                RunTime::deinitShim = 
                        (deinitShimFnType)dlsym (RTLD_NEXT, "deinitShim");
        }
	// make sure the shim is off -- if its lurking around
	if(RunTime::deinitShim) {
		((RunTime::deinitShim)());
	}
	load_flow();
	// create the initial thread list
	_num_threads = _rtc.initial_thread_pool_size;
	if(_num_threads <= flow()->sources_count()) {
		oflux_log_warn("RunTime::RunTime() forced bump up"
			"in thread count from %u to %u "
			"due to the number of sources\n"
			, _num_threads
			, flow()->sources_count()+1);
		_num_threads = flow()->sources_count()+1;
	}
	RunTimeThread ** rtt = &_threads;
	for(size_t i = 0; i < (size_t)_num_threads; ++i) {
		*rtt = new RunTimeThread(*this,i,(i ? 0 : oflux_self()));
		rtt = &((*rtt)->_next);
	}
	*rtt = NULL;
}

RunTime::~RunTime()
{
	// should have stopped all threads at this point
	RunTimeThread * rtt = _threads;
	RunTimeThread * rtt_next = NULL;
	_threads = NULL;
	while(rtt) {
		rtt_next = rtt->_next;
		rtt->_next = NULL;
		delete rtt;
		rtt = rtt_next;
	}
	ActiveFlow * aflow = _active_flow;
	ActiveFlow * next_aflow = NULL;
	_active_flow = NULL;
	while(aflow) {
		next_aflow = aflow->next;
		delete aflow->flow;
		delete aflow;
		aflow = next_aflow;
	}
}

void
RunTime::submitEvents(const std::vector<EventBasePtr> & evs)
{
	size_t t_ind = _tn.index;
	RunTimeThread * thread_ptr = _threads;
	while(thread_ptr && evs.size()) {
		if((int)t_ind == thread_ptr->index()) {
			thread_ptr->submitEvents(evs);
			return;
		}
		thread_ptr = thread_ptr->_next;
	}
	oflux_log_error("lockfree::RunTime::submitEvents dropped %d events\n", evs.size());
}

void 
RunTime::soft_kill()
{
	_request_death = true;
	bool dead;
	int retries = 4;
	if(_tn.index == 0) return; // you are thread 0 !
	while(retries > 0 && !(dead = _threads->die())) { // make the thread 0 exit
		--retries;
	}
	if(!dead) {
		oflux_log_warn("Runtime::soft_kill() failed to make thread 0 die!\n");
		exit(111); // desparate measures
	}
}

void
RunTime::distribute_events(RunTimeThread * rtt, std::vector<EventBasePtr> & events)
{
	// first find the current thread's rtt:
	while(rtt && rtt->index() != (int)_tn.index) {
		rtt = rtt->_next;
	}
	assert(rtt);
	// now put the events into the bottom of the queue
	for(size_t i = 0; i < events.size(); ++i) {
		rtt->pushLocal(events[i]);
	}
}

void  // TODO move this lower
RunTime::load_flow(
	  const char * flname
	, PluginSourceAbstract * pluginxmldir
	, const char * pluginlibdir
	, void * initpluginparams)
{
        oflux_log_trace("RunTime::load_flow() called\n");
	if(*flname == '\0') {
		flname = _rtc.flow_filename;
	}
	if(!pluginxmldir) {
		pluginxmldir = _rtc.plugin_name_source;
	}
	if(*pluginlibdir == '\0') {
		pluginlibdir = _rtc.plugin_lib_dir;
	}
        if(initpluginparams == NULL) {
                initpluginparams = _rtc.init_plugin_params;
        }
	// read XML file
        flow::Flow * flow = xml::read(
		  flname
		, _rtc.flow_maps
		, pluginxmldir
		, pluginlibdir
		, initpluginparams
		, this->flow()
		, atomics_style());
        flow->assignMagicNumbers(); // for guard ordering
	// push the sources (first time)
	if(_running) {
		std::vector<EventBasePtr> events_vec;
		event::push_initials_and_sources(events_vec, flow, true);
		distribute_events(_threads,events_vec); 
	}
	ActiveFlow * aflow = new ActiveFlow(flow);
	aflow->next = _active_flow;
	_active_flow = aflow;
	if(aflow->next && aflow->next->flow->sources().size() > 0) {
		aflow->next->flow->turn_off_sources();
	}
}

bool
RunTime::incr_sleepers()
{
	int sc = _sleep_count;
	while(sc + 1 < _num_threads) {
		if(__sync_bool_compare_and_swap(
				  &_sleep_count
				, sc
				, sc+1)) {
			return true;
		}
		sc = _sleep_count;
	}
	return false;
}

void
RunTime::decr_sleepers()
{
	__sync_fetch_and_sub(&_sleep_count,1);
}

struct sigaction __oldsigaction;
volatile bool __ignore_sig_int = false;

void __runtime_sigint_handler(int)
{
	oflux_log_info("Runtime sigint handler on thread %d %s\n"
		, _tn.index
		, (__ignore_sig_int ? "ignoring" : "default behaviour"));
	if(!__ignore_sig_int) {
		// default behaviour if we have not ask that the default be
		// temporarily ignored with __ignore_sig_int
		sigaction(SIGINT,&__oldsigaction,NULL);
		kill(0,SIGINT);
	}
}

#ifdef HAS_DOORS_IPC
# define OFLUX_SET_RT_FOR_DOORS doorthread::theRT = this
namespace doorthread {
  // things I wish were not global -- libdoor forces me to do this
  RunTime * theRT = NULL;
} // namespace doorthread

static void *
RunTimeThread_start_door_thread(void * pthis)
{
	RunTimeThread * rtt = static_cast<RunTimeThread*> (pthis);
	SetTrue keep_true_during_lifetime(rtt->_running);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	OFLUX_DOOR_RETURN;
	return NULL;
}


static void
RunTime_start_door_thread(door_info_t *)
{
	assert(doorthread::theRT);
	doorthread::theRT->setupDoorsThread();
}               
#else // ! HAS_DOORS_IPC
# define OFLUX_SET_RT_FOR_DOORS 
# define RunTime_start_door_thread
#endif // HAS_DOORS_IPC

void
RunTime::setupDoorsThread()
{
#ifdef HAS_DOORS_IPC
	if(!_doors_thread) {
		_doors_thread = new RunTimeThread(*this,1024,0);
		oflux_create_thread(
			  config().stack_size
			, RunTimeThread_start_door_thread
			, _doors_thread
			, &(_doors_thread->_tid));
	}
#endif // HAS_DOORS_IPC
}

void
RunTime::start()
{
	oflux_log_trace("RunTime::start() called\n");
	SetTrue st(_running);
	{
		std::vector<EventBasePtr> events_vec;
		event::push_initials_and_sources(events_vec, flow());
		distribute_events(_threads,events_vec); 
	}	
	RunTimeThread * this_rtt = _threads; 
	_thread = this_rtt;
		// [0] indexed thread is the initial thread.
	if(!this_rtt) return; // no threads allocated
	// setup all the threads after thread 0
	RunTimeThread * rtt = this_rtt->_next;
	// signal handler for SIGINT
	if(sigaction(SIGINT,NULL,&__oldsigaction) >= 0 
			&& (__oldsigaction.sa_handler == SIG_DFL
				|| __oldsigaction.sa_handler == SIG_IGN)) {
		struct sigaction newaction;
		memset(&newaction,0,sizeof(newaction));
		sigemptyset(&newaction.sa_mask);
		newaction.sa_handler = __runtime_sigint_handler;
		sigaction(SIGINT,&newaction,NULL);
	}
	// if doors, start up 
	OFLUX_SET_RT_FOR_DOORS;
	if(_doors.create_doors(RunTime_start_door_thread)) {
	}
	// start threads > 0
	int res = 0;
	while(rtt) {
		res = res || rtt->create();
		rtt = rtt->_next;
	}
	// start thread 0
	this_rtt->start();
	oflux_log_trace("thread index %d finished\n",this_rtt->index());
	// tear down phase
	rtt = _threads;
	rtt = rtt->_next;
	void * dontcare;
	while(rtt) {
		if(rtt->die()) { // successfully convinced the thread to die
			oflux_join(rtt->self(),&dontcare);
		}
		rtt = rtt->_next;
	}
}

void
RunTime::log_snapshot()
{
	oflux_log_info("RT %s %s %s nthrs:%d slp:%d\n"
		, _running ? "running" : "       "
		, _request_death ? "req-death" : "         "
		, _soft_load_flow ? "s-ld-flow" : "         "
		, _num_threads
		, _sleep_count);
	RunTimeThread * rtt = _threads;
	while(rtt) {
		rtt->log_snapshot();
		rtt = rtt->_next;
	}
	if(_doors_thread) {
		oflux_log_info("doors thread:\n");
		_doors_thread->log_snapshot();
	}
	oflux_log_info("RTend\n");
}

void
RunTime::log_snapshot_guard(const char *guardname)
{ 
	flow()->log_snapshot_guard(guardname);
}

void
RunTime::getPluginNames(std::vector<std::string> & result)
{
        flow::Flow * f = flow();
        if(f) f->getLibraryNames(result);
}

__thread RunTimeThread * RunTime::_thread;


} // namespace lockfree
} // namespace oflux
