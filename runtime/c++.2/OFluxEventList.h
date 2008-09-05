#ifndef _OFLUX_EVENT_LIST
#define _OFLUX_EVENT_LIST

#include "OFluxLinkedList.h"

namespace oflux {

class EventBase;

typedef Removable<EventBase, SharedPtrNode<EventBase> > EventListNode;
typedef LinkedListRemovable<EventBase, EventListNode> EventList;

} // namespace

#endif // _OFLUX_EVENT_LIST
