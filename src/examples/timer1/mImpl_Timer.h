#ifndef TIMER_MIMPL_H
#define TIMER_MIMPL_H

#include <boost/shared_ptr.hpp>
#include "OFluxThreads.h"
#include "atomic/OFluxAtomicInit.h"
#include <signal.h>

#define OFLUX_TIMER_INIT(TIMERINST) \
  { \
  OFLUX_GUARD_POPULATER(TIMERINST##_State,StatePop); \
  Timer::State_key sk; \
  StatePop.insert(&sk, Timer::factory_State()); \
  }

// todo: write the plugin one too (for the above)

namespace Timer {

inline bool isTrue(bool v) { return v; }
 
/**
 * @class EventBase
 * @brief base class for Timer events (maintains basic properties: id and expiry)
 *   Users of the Timer should subclass this object to ride their own data into event 
 *  objects
 */
class EventBase {
public:
	EventBase(size_t id = unique_id_generator())
		: _id(id)
		, _expiry_time(0)
	{}
	virtual ~EventBase() {}
	virtual size_t id() const { return _id; } 

	time_t expiry() { return _expiry_time; }
	void expiry(time_t t) { _expiry_time = t; }

	static size_t unique_id_generator();
protected:
	size_t _id;
	time_t _expiry_time;
};

typedef boost::shared_ptr<EventBase> EventBasePtr;

class State;

extern State *
factory_State(int sig = SIGALRM);


} // namespace Timer

#endif // TIMER_MINPL_H
