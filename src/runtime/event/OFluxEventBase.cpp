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
