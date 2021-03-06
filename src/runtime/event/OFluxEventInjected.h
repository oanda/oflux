#ifndef OFLUX_EVENT_INJECTED_H
#define OFLUX_EVENT_INJECTED_H
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

#include "event/OFluxEventDoor.h"
#include "flow/OFluxFlow.h"
#include "OFluxIOConversion.h"
#include <string>

// Given a node Foo.
// This is a way to get events into the runtime queue on the side:
// 
// using namespace oflux;
// RunTimeAbstract * theRT = ...
// std::vector<EventBasePtr> evs;
// Foo_in input; ... set values
// EventBasePtr ev = create_injected<FooDetail>(input,rt_lookup_flow_node("Foo",theRT));
// if(ev) { evs.push_back(ev); }
// ... repeat as needed
// theRT->submitEvents(evs);
//
// There is a map lookup to get the flow::Node * to do this, which is not 
// the most efficient thing ever, but you can see to doing that only once in a
// static (e.g. static flow::Node * fn = rt_lookup_flow_node(...); )

namespace oflux {

namespace flow {
 class Node;
} // namespace flow

flow::Node *
rt_lookup_flow_node(
	  const std::string & node_name
        , RunTimeAbstract * rt);

template<typename D>
struct PredDetail {
	typedef typename D::In_ Out_;
	struct In_ {};
	struct Atoms_ {};
};

namespace injected {
 extern flow::Node * fn_for_injection;
} // namespace injected

template<typename D>
inline EventBasePtr
create_injected(
	  const typename D::In_ & in // in structure for node
	, flow::Node * fn)
{
	if(!fn) {
		return EventBasePtr(); // may change to throw
	}
	DoorEvent<PredDetail<D> > * doorev =
		new DoorEvent<PredDetail<D> >(&in,sizeof(in),injected::fn_for_injection);
	OutputWalker ow = doorev->output_type();
	EventBaseSharedPtr predecessor(doorev);
	return create<D>(
		  predecessor
		, new TrivialIOConversion<typename D::In_>(reinterpret_cast<typename D::In_ *>(ow.next()))
		, fn);
}


} // namespace oflux

#define OFLUX_DECLARE_INJECTION_VECTOR(E) \
   std::vector<oflux::EventBasePtr> E

#define ENAMESTR(X) #X

#define OFLUX_INSERT_INJECTION_INPUT(E,EN,I) \
   { \
       static oflux::flow::Node * fn = oflux::rt_lookup_flow_node(ENAMESTR(EN), theRT.get()); \
       oflux::EventBasePtr ev = oflux::create_injected< EN##Detail >(I,fn); \
       E.push_back(ev); \
   }

#define OFLUX_SUBMIT_INJECTION_VECTOR(E) \
   theRT->submitEvents(E)


#endif //OFLUX_EVENT_INJECTED_H
