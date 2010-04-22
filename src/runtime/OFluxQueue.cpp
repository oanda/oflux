#include "OFluxLogging.h"
#include "OFluxQueue.h"
#include "event/OFluxEventBase.h"
#include "OFluxLibDTrace.h"


namespace oflux {

const char *
Queue::element_name(Element & e)
{
	return flow::get_node_name(e->flow_node());
}

void
Queue::push(Element & e)
{
	_q.push_back(e);
	_FIFO_PUSH(e.get(),element_name(e));
}

void
Queue::push_priority(Element & e)
{
	_q.push_front(e);
	_FIFO_PUSH(e.get(),element_name(e));
}
void
Queue::push_list(std::vector<Element> & vec)
{
	for(int i = 0; i < (int)vec.size(); i++) {
		_q.push_back(vec[i]);
		_FIFO_PUSH(vec[i].get(),element_name(vec[i]));
	}
}
void
Queue::push_list_priority(std::vector<Element> & vec)
{
	// reverse
	for(int i = ((int)vec.size())-1; i >= 0; i--) {
		_q.push_front(vec[i]);
		_FIFO_PUSH(vec[i].get(),element_name(vec[i]));
	}
}

bool
Queue::pop(Element & e)
{
	bool res = _q.size() > 0;
	if(res) {
		e = _q.front();
		_q.pop_front();
		_FIFO_POP(e.get(),element_name(e));
	}
	return res;
}

void
Queue::log_snapshot()
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
