#include "OFluxEvent.h"
#include "OFluxLogging.h"
//#include "OFluxFlow.h"

/**
 * @file OFluxEvent.cpp
 */

namespace oflux {

/**
 * @brief need an event that is sort of the empty event
 * This is used as a predecessor of a successor.
 */
boost::shared_ptr<EventBase> EventBase::no_event;

void EventBase::log_snapshot()
{
	int atomics_number = _atomics.number();
	int held_atomics = 0;
	for(int i= 0; i < atomics_number; i++) {
		held_atomics += (_atomics.get(i)->haveit() ? 1 : 0);
	}
	oflux_log_info(" %s (%d/%d atomics)\n", flow_node()->getName(),
		held_atomics,
		atomics_number);
	// could print the names of the atomics held...
}

};
