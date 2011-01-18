#ifndef OFLUX_EARLY_RELEASE
#define OFLUX_EARLY_RELEASE

//
// Normally the guards of a node are held for the duration of the node function execution.
// This interface allows you to release them earlier than that (which can be good to reduce contention
// (particularly in a lock-free setting)

namespace oflux {

class RunTimeAbstract;

void release_all_guards(RunTimeAbstract * rt); 
	// must be called within the dynamic scope of a node function
	// not within the runtime proper!

} // namespace oflux

#endif // OFLUX_EARLY_RELEASE
