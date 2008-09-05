#include "OFluxLogging.h"
#include "OFluxQueue.h"
#include "OFluxFlow.h"
#include "OFluxEvent.h"

namespace oflux {

void Queue::push_list(EventList & ll, FlowNode * source_flow_node) {
	if(source_flow_node != NULL && _prioritize_queued_sources) {
		_q_sources.insert_back_all(ll);
	} else {
		EventListNode * n = ll.first();
		while(n != NULL) {
			Element & ele = n->shared_content();
			push(ele,ele->flow_node() == source_flow_node);
			n = n->next();
		}
	}
}

void Queue::log_snapshot()
{
	EventListNode * eln = _q_sources.first();
	oflux_log_info("<front of the event queue here>\n");
	while(eln != NULL) {
		eln->content()->log_snapshot();
		eln = eln->next();
	}
	if(_prioritize_queued_sources) {
		oflux_log_info("<nonsources event queue starts here>\n");
	}
	eln = _q_nonsources.first();
	while(eln != NULL) {
		eln->content()->log_snapshot();
		eln = eln->next();
	}
	oflux_log_info("<back of the event queue here>\n");
}

} // namespace oflux

