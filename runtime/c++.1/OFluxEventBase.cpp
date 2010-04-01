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

EventBase::EventBase(        EventBasePtr & predecessor
		, flow::Node *flow_node)
	: flow::NodeCounterIncrementer(flow_node)
	, _predecessor(predecessor)
	, _error_code(0)
	, _atomics(flow_node->isGuardsCompletelySorted())
{
	std::vector<flow::GuardReference *> & vec = 
		flow_node->guards();
	for(int i = 0; i < (int) vec.size(); i++) {
		_atomics.add(vec[i]);
	}
	PUBLIC_EVENT_BORN(this,flow_node->getName());
}

EventBase::~EventBase()
{
	PUBLIC_EVENT_DEATH(this,flow_node()->getName());
}

void
EventBase::release(std::vector<EventBasePtr> & released_events)
{
	_atomics.release(released_events);
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


int
EventBase::acquire_guards(
	  EventBasePtr & ev
	, AtomicsHolder & ah)
{
	enum { AGR_Success = 1, AGR_MustWait = 0 };
	int res = AGR_Success;
	EventBase * ev_bptr = ev.get();
	const char * ev_name = ev_bptr->flow_node()->getName();
        _NODE_ACQUIREGUARDS(
		  static_cast<void *>(ev_bptr)
		, ev_name);
	AtomicsHolder & atomics = ev_bptr->atomics();
	int ah_res = atomics.acquire(
		  ah
		, ev_bptr->input_type()
		, ev_name);
	HeldAtomic * held_atomic = 
		( ah_res == -1 
		? NULL
		: atomics.get(ah_res) );
	int wtype = (held_atomic ? held_atomic->wtype() : 0);
	flow::GuardReference * flow_guard_ref __attribute__((unused)) = 
		(held_atomic ? held_atomic->flow_guard_ref() : NULL);
	Atomic * must_wait_on_atomic = 
		(held_atomic ? held_atomic->atomic() : NULL);
	if(must_wait_on_atomic) {
		res = AGR_MustWait;
		must_wait_on_atomic->wait(ev,wtype);
		_GUARD_WAIT(
			  flow_guard_ref->getName().c_str()
			, ev_name
			, wtype);
	} else {
                _NODE_HAVEALLGUARDS(
			  static_cast<void *>(ev_bptr)
			, ev_name);
        }
	return res;
}

AtomicsHolder & EventBase::empty_ah = AtomicsHolder::empty_ah;

};
