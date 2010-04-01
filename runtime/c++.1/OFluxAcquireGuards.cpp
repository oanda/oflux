#include "OFluxAcquireGuards.h"
#include "OFluxLibDTrace.h"

namespace oflux {

AtomicsHolder AcquireGuards::empty_ah;

AcquireGuards::Result
AcquireGuards::doit(
	  EventBasePtr & ev
	, AtomicsHolder & ah)
{
	Result res = AGR_Success;
        _NODE_ACQUIREGUARDS(static_cast<void *>(ev.get())
		,ev->flow_node()->getName());
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
	} else {
                _NODE_HAVEALLGUARDS(static_cast<void *>(ev.get())
			, ev->flow_node()->getName());
        }
	return res;
}

} // namespace oflux
