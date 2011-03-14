#ifndef _OFLUX_EVENTBASE_H
#define _OFLUX_EVENTBASE_H


/**
 * @file OFluxEvent.h
 * @author Mark Pichora
 *  This is the header file for the Event (base class).
 *  This is the bottom layer which is sufficient for the runtime (in terms
 *  of what is accessible
 */

#include "OFlux.h"
#include "flow/OFluxFlowNodeIncr.h"
#include <vector>


namespace oflux {

namespace atomic {
 class AtomicsHolder;
} // namespace atomic

#ifdef HAS_DTRACE
void PUBLIC_EVENT_BORN(const void *,const char * X);
void PUBLIC_EVENT_DEATH(const void *,const char * X);
void PUBLIC_NODE_START(const void *,const char * X,int Y,int Z);
void PUBLIC_NODE_DONE(const void *,const char * X);
#else // HAS_DTRACE
# define PUBLIC_NODE_START(E,X,Y,Z)
# define PUBLIC_NODE_DONE(E,X)
# define PUBLIC_EVENT_BORN(E,X)
# define PUBLIC_EVENT_DEATH(E,X)
#endif // HAS_DTRACE

/**
 * @class EventBase
 * @brief the event base class which covers the low level common operations
 * Events hold a reference to the previous event to hold onto the memory.
 * This keeps it from being reclaimed to the heap before the current event
 * is done executing.
 */
class EventBase : public flow::NodeCounterIncrementer {
public:
	static EventBaseSharedPtr no_event_shared;
	static EventBasePtr no_event;

	EventBase(        EventBaseSharedPtr & predecessor
			, flow::Node *flow_node
			, atomic::AtomicsHolder & atomics);
	virtual ~EventBase();
	virtual OutputWalker output_type() = 0;
	virtual const void * input_type() = 0;
	void release() { _predecessor = no_event; }
	virtual int execute() = 0;
	/**
	 * @brief obtain the events from releasing your held guards 
	 *    -- this pulls things off the wait-lists
	 */
	//void release(std::vector<EventBasePtr> & released_events);
	inline void error_code(int ec) { _error_code = ec; }
	inline int error_code() { return _error_code; }
	inline atomic::AtomicsHolder & atomics() { return _atomics_ref; }
	void log_snapshot();
	inline EventBaseSharedPtr & get_predecessor()
	{ return _predecessor; }
	bool getIsDetached();
private:
	EventBaseSharedPtr _predecessor;
protected:
	int _error_code;
	atomic::AtomicsHolder & _atomics_ref;
public:
	int state;
};


} // namespace oflux

#endif // _OFLUX_EVENTBASE_H
