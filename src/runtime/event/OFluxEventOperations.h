#ifndef OFLUX_EVENT_OPERATIONS_H
#define OFLUX_EVENT_OPERATIONS_H
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
 * @file OFluxEventOperations.h
 * @author Mark Pichora
 *  Common operations on events which require information on flow and atomics 
 *  held by a node and its predecessor.
 */

#include "OFlux.h"
#include <vector>

namespace oflux {

namespace atomic {
 class AtomicsHolder;
} // namespace atomic

namespace flow {
 class Flow;
} // namespace flow

namespace event {

extern int 
acquire_guards(
	  EventBasePtr & ev
	, EventBasePtr & pred_ev);


/**
 * @brief compute and return the successor_events from predecessor ev when
 *    no error was encountered (predecessor's function returned 0).
 * @param successor_events is the vector where results are appended
 * @param ev is the predecessor event
 */
void
successors_on_no_error(
	  std::vector<EventBasePtr> & successor_events
	, EventBaseSharedPtr & ev);


/**
 * @brief compute and return the successor_events from predecessor ev when
 *    an error was encountered (predecessor's function returned non-0).
 * @param successor_events is the vector where results are appended
 * @param ev is the predecessor event
 */
void
successors_on_error(
	  std::vector<EventBasePtr> & successor_events
	, EventBaseSharedPtr & ev
	, int return_code);

/**
 * @brief push onto the events_vec the set of initial events and source events
 *    which the program needs to begin.  The events should then end up on the
 *    runtime queue.
 * @param events_vec vector where the events are appended
 * @param flow is the program Flow object
 * @param lifo indicates if the runtime queue is LIFO
 */
void
push_initials_and_sources( 
	  std::vector<EventBasePtr> & events_vec
	, flow::Flow * flow
	, bool lifo = false);

} // namespace event
} // namespace oflux


#endif // OFLUX_EVENT_OPERATIONS
