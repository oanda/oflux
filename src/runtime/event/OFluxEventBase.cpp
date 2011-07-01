#include "event/OFluxEventBase.h"
#include "flow/OFluxFlowNode.h"
#include "OFluxLogging.h"
#include "OFluxLibDTrace.h"
#include "atomic/OFluxAtomicHolder.h"
/**
 * @file OFluxEventBase.cpp
 * @brief simple things in the base object part of the event (type independent)
 */

namespace oflux {

/**
 * @brief need an event that is sort of the empty event
 * This is used as a predecessor of a successor.
 */
EventBasePtr EventBase::no_event(NULL);
EventBaseSharedPtr EventBase::no_event_shared;


void 
EventBase::log_snapshot()
{
	int atomics_number = _atomics_ref.number();
	int held_atomics = 0;
	for(int i= 0; i < atomics_number; i++) {
		held_atomics += (_atomics_ref.get(i)->haveit() ? 1 : 0);
	}
	oflux_log_info(" %s (%d/%d atomics)\n", flow_node()->getName(),
		held_atomics,
		atomics_number);
	// could print the names of the atomics held...
}

EventBase::EventBase( EventBaseSharedPtr & predecessor
		, flow::Node *flow_node
		, atomic::AtomicsHolder & atomics)
	: flow::NodeCounterIncrementer(flow_node)
	, _predecessor(predecessor)
	, _error_code(0)
	, _atomics_ref(atomics)
	, state(0)
{
	PUBLIC_EVENT_BORN(this,flow_node->getName());
}

EventBase::~EventBase()
{
	PUBLIC_EVENT_DEATH(this,flow_node()->getName());
}


bool
EventBase::getIsDetached()
{ 
	return flow_node()->getIsDetached();
}



} // namespace oflux
