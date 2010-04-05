#ifndef OFLUX_EVENT_OPERATIONS_H
#define OFLUX_EVENT_OPERATIONS_H

#include "OFlux.h"
#include <vector>

namespace oflux {

class AtomicsHolder;

namespace flow {
class Flow;
} // namespace flow

namespace event {

extern AtomicsHolder & empty_ah;

/**
 * @brief either acquire all guards or enqueue onto the wait-list
 *        for the [first] guard you could not get
 * @param given the atomics holder from which to canabalized guards
 *        from (the predecessor event normally).
 * @return 1 on success and 0 on failure
 */
int acquire_guards(EventBasePtr &, AtomicsHolder & ah = empty_ah);

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
	, flow::Flow * flow);

} // namespace event
} // namespace oflux


#endif // OFLUX_EVENT_OPERATIONS
