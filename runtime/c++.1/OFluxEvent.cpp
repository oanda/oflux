#include "OFluxEvent.h"
#include "OFluxLogging.h"
#include "OFluxLibDTrace.h"
/**
 * @file OFluxEvent.cpp
 */

namespace oflux {

#ifdef HAS_DTRACE
void PUBLIC_NODE_START(const void * E,const char * X,int Y,int Z)
{
	OFLUX_NODE_START(const_cast<void *>(E),const_cast<char *>(X),Y,Z);
}

void PUBLIC_NODE_DONE(const void * E,const char * X)
{
	OFLUX_NODE_DONE(const_cast<void *>(E),const_cast<char *>(X));
}

void PUBLIC_EVENT_BORN(const void * E, const char * N)
{
        OFLUX_EVENT_BORN(const_cast<void *>(E),const_cast<char *>(N));
}

void PUBLIC_EVENT_DEATH(const void * E, const char * N)
{
        OFLUX_EVENT_DEATH(const_cast<void *>(E),const_cast<char *>(N));
}

#endif // HAS_DTRACE

/**
 * @brief need an event that is sort of the empty event
 * This is used as a predecessor of a successor.
 */
boost::shared_ptr<EventBase> EventBase::no_event;

#ifdef HAS_DTRACE
int access_dtrace()
{
	char * empty = const_cast<char *>("");
	OFLUX_NODE_START(NULL,empty,0,0);
	OFLUX_NODE_DONE(NULL,empty);
	OFLUX_NODE_HAVEALLGUARDS(NULL,empty);
	OFLUX_NODE_ACQUIREGUARDS(NULL,empty);
	OFLUX_EVENT_BORN(NULL,empty);
	OFLUX_EVENT_DEATH(NULL,empty);
	//OFLUX_SHIM_CALL(empty);
	//OFLUX_SHIM_WAIT(empty);
	//OFLUX_SHIM_RETURN(empty);
	return empty == NULL;
}
#endif // HAS_DTRACE

void EventBase::log_snapshot()
{
#ifdef HAS_DTRACE
	static int ok_keep_dtrace_symbols __attribute__((unused)) = access_dtrace();
#endif // HAS_DTRACE
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
