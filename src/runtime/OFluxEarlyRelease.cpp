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
#include "OFluxEarlyRelease.h"
#include "OFlux.h"
#include "OFluxSharedPtr.h"
#include "event/OFluxEventBase.h"
#include "atomic/OFluxAtomicHolder.h"
#include "OFluxRunTimeAbstract.h"
#include "OFluxRunTimeThreadAbstract.h"
#include "flow/OFluxFlowNode.h"
#include "OFluxLogging.h"
#include <vector>


namespace oflux {

void
release_all_guards(RunTimeAbstract * rt)
{
	RunTimeThreadAbstract * rtt = rt->thread();
	EventBase * evb = rtt->thisEvent();
	std::vector<EventBasePtr> rel_evs;
	EventBasePtr by_ev(evb);
	evb->atomics().release(rel_evs,by_ev);
	recover_EventBasePtr((void)by_ev);  // disable reclaimation
	// now resubmit events which have fully acquired 
	std::vector<EventBasePtr> rel_evs_full_acq;
	for(size_t i = 0; i < rel_evs.size(); ++i) {
                EventBasePtr & succ_ev = rel_evs[i];
		EventBase * succ_evb = get_EventBasePtr(succ_ev);
                if(succ_evb->atomics().acquire_all_or_wait(succ_ev)) {
                        rel_evs_full_acq.push_back(succ_ev);
                } else {
                        oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] acquire_all_or_wait() failure for "
                                "%s %p on guards acquisition"
                                , oflux_self()
                                , succ_evb->flow_node()->getName()
                                , succ_evb);
                }

	}
	rtt->submitEvents(rel_evs_full_acq);
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] "
		"oflux::release_all_guards for ev %s %p released "
		"%u events prematurely enqueued %u\n"
		, oflux_self()
		, evb->flow_node()->getName()
		, evb
		, rel_evs.size()
		, rel_evs_full_acq.size()
		);
}

} // namespace oflux

