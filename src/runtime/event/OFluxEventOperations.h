#ifndef OFLUX_EVENT_OPERATIONS_H
#define OFLUX_EVENT_OPERATIONS_H

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

void
successors_on_no_error(
	  std::vector<EventBasePtr> & successor_events
	, EventBasePtr & ev);


void
successors_on_error(
	  std::vector<EventBasePtr> & successor_events
	, EventBasePtr & ev
	, int return_code);

void
push_initials_and_sources( 
	  std::vector<EventBasePtr> & events_vec
	, flow::Flow * flow
	, bool lifo = false);

} // namespace event
} // namespace oflux


#endif // OFLUX_EVENT_OPERATIONS
