#ifndef OFLUX_RUNTIME_THREAD_ABSTRACT_H
#define OFLUX_RUNTIME_THREAD_ABSTRACT_H
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
