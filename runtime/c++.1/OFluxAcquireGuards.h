#ifndef _OFLUX_ACQUIRE_GUARDS
#define _OFLUX_ACQUIRE_GUARDS

#include "OFluxAtomicHolder.h"
#include "OFluxFlow.h"
#include "OFluxEvent.h"

namespace oflux {

class AcquireGuards {
public:

enum AcquireGuardsResult { AGR_Success = 1, AGR_MustWait = 0 };

static AtomicsHolder empty_ah;

static inline AcquireGuardsResult
doit(boost::shared_ptr<EventBase> & ev, AtomicsHolder & ah = empty_ah)
{
	AcquireGuardsResult res = AGR_Success;
	int wtype = 0;
	flow::GuardReference * flow_guard_ref = NULL;
	Atomic * must_wait_on_atomic = 
		ev->acquire(wtype /*output*/,
			flow_guard_ref /*output*/,
			ah);
	if(must_wait_on_atomic) {
		res = AGR_MustWait;
		must_wait_on_atomic->wait(ev,wtype);
		_GUARD_WAIT(flow_guard_ref->getName().c_str(),
			ev->flow_node()->getName(), 
			wtype);
	}
	return res;
}
}; // AcquireGuards class

} // namespace oflux
	

#endif // _OFLUX_ACQUIRE_GUARDS
