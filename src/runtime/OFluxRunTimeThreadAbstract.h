#ifndef OFLUX_RUNTIME_THREAD_ABSTRACT_H
#define OFLUX_RUNTIME_THREAD_ABSTRACT_H

#include <vector>
#include "OFlux.h"
#include "OFluxProfiling.h"
#include "OFluxThreads.h"

namespace oflux {

enum RTT_WaitState 
	{ RTTWS_running = 0
	, RTTWS_wtr = 1
	, RTTWS_wip = 2
	, RTTWS_blockingcall = 3
	, RTTWS_wtrshim = 4 
	, RTTWS_door_service = 5 
	, RTTWS_door_wait = 6 
	};


class RunTimeThreadAbstract {
public:
	virtual ~RunTimeThreadAbstract() {}
	virtual bool is_detached() = 0;
	virtual void set_detached(bool d) = 0;
#ifdef PROFILING
	virtual TimerList & timer_list() = 0;
#endif
	virtual void wait_state(RTT_WaitState) = 0;
	virtual oflux_thread_t tid() = 0;

	virtual void submitEvents(const std::vector<EventBasePtr> &) = 0;
	virtual EventBase * thisEvent() const = 0;
}; 

}// namespace oflux


#endif
