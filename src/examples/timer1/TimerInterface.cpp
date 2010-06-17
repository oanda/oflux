#include "TimerInterface.h"
#include "mImpl_Timer.h"
#include "mImpl_Timer_internal.h"

namespace Timer {

Submitter::Submitter(State &s)
	: _state(s)
{
}

Submitter::~Submitter()
{
	_state.signal();
}

void
Submitter::add(EventBase * eb)
{
	_state.asynch_add(eb);
}

void
Submitter::cancel(size_t id)
{
	_state.asynch_cancel(id);
}

} // namespace Timer

