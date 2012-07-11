#ifndef _OFLUX_EVENTBASE_H
#define _OFLUX_EVENTBASE_H
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
 * @file OFluxEvent.h
 * @author Mark Pichora
 *  This is the header file for the Event (base class).
 *  This is the bottom layer which is sufficient for the runtime (in terms
 *  of what is accessible
 */

#include "OFlux.h"
#include "flow/OFluxFlowNodeIncr.h"
#include <vector>


namespace oflux {

namespace atomic {
 class AtomicsHolder;
} // namespace atomic

#ifdef HAS_DTRACE
#else // HAS_DTRACE
#endif // HAS_DTRACE

/**
 * @class EventBase
 * @brief the event base class which covers the low level common operations
 * Events hold a reference to the previous event to hold onto the memory.
 * This keeps it from being reclaimed to the heap before the current event
 * is done executing.
 */
class EventBase : public flow::NodeCounterIncrementer {
public:
	static EventBaseSharedPtr no_event_shared;
	static EventBasePtr no_event;

	EventBase(        EventBaseSharedPtr & predecessor
			, flow::Node *flow_node
			, atomic::AtomicsHolder & atomics);
	virtual ~EventBase();
	virtual OutputWalker output_type() = 0;
	virtual const void * input_type() = 0;
	void release() { _predecessor = no_event; }
	virtual int execute() = 0;
	/**
	 * @brief obtain the events from releasing your held guards 
	 *    -- this pulls things off the wait-lists
	 */
	//void release(std::vector<EventBasePtr> & released_events);
	inline void error_code(int ec) { _error_code = ec; }
	inline int error_code() { return _error_code; }
	inline atomic::AtomicsHolder & atomics() { return _atomics_ref; }
	void log_snapshot();
	inline EventBaseSharedPtr & get_predecessor()
	{ return _predecessor; }
	bool getIsDetached();
private:
	EventBaseSharedPtr _predecessor;
protected:
	int _error_code;
	atomic::AtomicsHolder & _atomics_ref;
public:
	int state;
};


} // namespace oflux

#endif // _OFLUX_EVENTBASE_H
