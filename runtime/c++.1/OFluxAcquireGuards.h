#ifndef _OFLUX_ACQUIRE_GUARDS
#define _OFLUX_ACQUIRE_GUARDS

/**
 * @file OFluxAcquireGuards.h
 * @author Mark Pichora
 *  This class defines a static function that encapsulates all that
 *  is needed in the top-level runtime to acquire and hold dereferenced
 *  guards (aka Atomics).
 */

#include "OFluxAtomicHolder.h"
#include "OFluxFlow.h"
#include "OFluxEvent.h"

namespace oflux {

/**
 * @class AcquireGuards
 * @brief a class to hold the functionality for grabbing guards for
 *  a particular event.
 */
class AcquireGuards {
public:

enum AcquireGuardsResult { AGR_Success = 1, AGR_MustWait = 0 };

static AtomicsHolder empty_ah;

/**
 * @brief try to get all the atomics for the supplied event
 * @param ev the event
 * @param ah the held atomics being passed for free to this event (predecessor)
 * @return AGR_Success if we get them all, 
 *     and AGR_MustWait otherwise (so enqueue onto the waiter list 
 *         for the first atomic it failed to acquire)
 **/
static inline AcquireGuardsResult
doit(boost::shared_ptr<EventBase> & ev, AtomicsHolder & ah = empty_ah)
{
	AcquireGuardsResult res = AGR_Success;
        _NODE_ACQUIREGUARDS(ev->flow_node()->getName());
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
                _NODE_HAVEALLGUARDS(ev->flow_node()->getName());
        }
	return res;
}
}; // AcquireGuards class

} // namespace oflux
	

#endif // _OFLUX_ACQUIRE_GUARDS
