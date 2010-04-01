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
#include "OFluxFlow.h"
#include "OFluxAtomicHolder.h"
#include <vector>


namespace oflux {

class AtomicsHolder;

#ifdef HAS_DTRACE
void PUBLIC_EVENT_BORN(const void *,const char * X);
void PUBLIC_EVENT_DEATH(const void *,const char * X);
void PUBLIC_NODE_START(const void *,const char * X,int Y,int Z);
void PUBLIC_NODE_DONE(const void *,const char * X);
#else
# define PUBLIC_NODE_START(E,X,Y,Z)
# define PUBLIC_NODE_DONE(E,X)
# define PUBLIC_EVENT_BORN(E,X)
# define PUBLIC_EVENT_DEATH(E,X)
#endif

/**
 * @class EventBase
 * @brief the event base class which covers the low level common operations
 * Events hold a reference to the previous event to hold onto the memory.
 * This keeps it from being reclaimed to the heap before the current event
 * is done executing.
 */
class EventBase : public flow::NodeCounterIncrementer {
public:
	static EventBasePtr no_event;
	static AtomicsHolder & empty_ah;

	EventBase(        EventBasePtr & predecessor
			, flow::Node *flow_node);
	virtual ~EventBase();
	virtual OutputWalker output_type() = 0;
	virtual const void * input_type() = 0;
	void release() { _predecessor = no_event; }
	virtual int execute() = 0;
	/**
	 * @brief either acquire all guards or enqueue onto the wait-list
	 *        for the [first] guard you could not get
	 * @param given the atomics holder from which to canabalized guards
	 *        from (the predecessor event normally).
	 * @return 1 on success and 0 on failure
	 */
	static int acquire_guards(EventBasePtr &, AtomicsHolder & ah = empty_ah);
	/**
	 * @brief obtain the events from releasing your held guards 
	 *    -- this pulls things off the wait-lists
	 */
	void release(std::vector<EventBasePtr> & released_events);
	inline void error_code(int ec) { _error_code = ec; }
	inline int error_code() { return _error_code; }
	inline AtomicsHolder & atomics() { return _atomics; }
	void log_snapshot();
	inline EventBasePtr & get_predecessor()
	{ return _predecessor; }
private:
	EventBasePtr _predecessor;
protected:
	int           _error_code;
	AtomicsHolder _atomics;
};

} // namespace oflux

#endif // _OFLUX_EVENTBASE_H
