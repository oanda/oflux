#ifndef OFLUX_EVENT_DOOR_H
#define OFLUX_EVENT_DOOR_H
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
