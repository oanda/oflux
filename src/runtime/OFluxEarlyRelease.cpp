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
	by_ev.recover();  // disable reclaimation
	// now resubmit events which have fully acquired 
	std::vector<EventBasePtr> rel_evs_full_acq;
	for(size_t i = 0; i < rel_evs.size(); ++i) {
                EventBasePtr & succ_ev = rel_evs[i];
		EventBase * succ_evb = succ_ev.get();
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

