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
EventBasePtr EventBase::no_event;

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

EventBase::EventBase(        EventBasePtr & predecessor
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

/*
void
EventBase::release(std::vector<EventBasePtr> & released_events)
{
	_atomics_ref.release(released_events);
}
*/

bool
EventBase::getIsDetached()
{ 
	return flow_node()->getIsDetached();
}


/*
inline Atomic * 
EventBase__acquire( 
	  EventBase * pthis
	, int & wtype
	, flow::GuardReference * & fgr
	, AtomicsHolder & given_atomics)
{
	int res = _atomics.acquire(
			  given_atomics
			, input_type()
			, pthis->flow_node()->getName());
	HeldAtomic * ha = (res == -1 ? NULL : pthis->atomics().get(res));
	wtype = (ha ? ha->wtype() : 0);
	fgr = (ha ? ha->flow_guard_ref() : NULL);
	return (ha ? ha->atomic() : NULL);
}*/


}
