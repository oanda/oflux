#include "flow/OFluxFlowGuard.h"
#include "OFluxLogging.h"
#include <vector>


namespace oflux {
namespace flow {

void
GuardReference::log_aha_warning(const char *s, int arg)
{
	oflux_log_warn("oflux::flow::GuardReference::get() threw "
		"an AHAException -- so an earlier guard "
		"was not aquired %s, guard arg %d\n"
		, s
		, arg);
}

void
Guard::drain()
{
        std::vector<EventBasePtr> rel_vector;
        boost::shared_ptr<atomic::AtomicMapWalker> walker(_amap->walker());
        const void * v = NULL;
        atomic::Atomic * a = NULL;
	EventBasePtr nullPtr; 
		// don't have the original events
		// Atomic::release needs to handle by_ev == NULL
        while(walker->next(v,a)) {
                a->release(rel_vector,nullPtr);
        }
}

} // namespace flow
} // namespace oflux
