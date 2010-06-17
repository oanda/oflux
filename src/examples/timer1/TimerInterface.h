#ifndef TIMER_INTERFACE_H
#define TIMER_INTERFACE_H

#include <cstddef>

namespace Timer {

class State;

class EventBase;

/**
 * @class Submitter
 * @brief used to create a modification object for the Timer::state
 *     the initialization and destruction of this object does something to the state.
 *     It makes the interaction with the Timer::State more efficient.
 */
class Submitter {
public:
	Submitter(State &);
	~Submitter();
	void add(EventBase *);
	void cancel(size_t);
private:
	State & _state;
};

} // namespace Timer

#endif
