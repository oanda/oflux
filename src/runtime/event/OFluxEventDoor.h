#ifndef OFLUX_EVENT_DOOR_H
#define OFLUX_EVENT_DOOR_H

/**
 * @file OFluxEventDoor.h
 * @author Mark Pichora
 *  This is the header file for events that are created from door IPC.
 *  Only systems with support for doors should compile this into their runtime.
 */

#include "event/OFluxEvent.h"
#include <string.h>

namespace oflux {

template< typename Detail >
class DoorEvent : public EventBaseTyped<Detail> {
public:
	DoorEvent(const void * ptr, size_t sz, flow::Node *fn) 
		: EventBaseTyped<Detail>(EventBase::no_event_shared, NULL, fn) 
	{
		assert(sz == sizeof(typename Detail::Out_));
		memcpy(EventBaseTyped<Detail>::pr_output_type(), ptr, sz);
		EventBaseTyped<Detail>::pr_output_type()->next(NULL);
	}
	virtual int execute() { return 0; } // not called anyway
};


	 

} // namespace oflux


#endif // OFLUX_EVENT_DOOR_H
