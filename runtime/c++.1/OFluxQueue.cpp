#include "OFluxLogging.h"
#include "OFluxQueue.h"
#include "OFluxEvent.h"

namespace oflux {

const char * Queue::element_name(Element & e)
{
	return e->flow_node()->getName();
}

void Queue::log_snapshot()
{
	std::deque<Element>::iterator dqitr = _q.begin();
	std::deque<Element>::iterator dqitr_end = _q.end();
	int sz = _q.size();
	oflux_log_info("<front of the event queue here>\n");
	while(dqitr != dqitr_end && sz > 0) {
		(*dqitr)->log_snapshot();
		dqitr++;
		sz--; // done for safety -- never know who will access this in MT (you are bad ppl!)
	}
	oflux_log_info("<back of the event queue here>\n");
}

} // namespace oflux

