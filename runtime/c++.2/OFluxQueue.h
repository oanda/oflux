#ifndef _OFLUX_QUEUE
#define _OFLUX_QUEUE

/**
 * @filename OFluxQueue.h
 * @author Mark Pichora
 * So far this run time only has a FIFO queue.
 */

#include "OFlux.h"
#include "boost/shared_ptr.hpp"
//#include <deque>
//#include <vector>
#include "OFluxEventList.h"

// queue for events (FIFO)


namespace oflux {



/**
 * @class Queue
 * @brief the run time's queue data structure
 */
class Queue {
public:
	typedef boost::shared_ptr<EventBase> Element;
	Queue(bool prioritize_queued_sources) 
		: _prioritize_queued_sources(prioritize_queued_sources)
		{}
	~Queue() {}
	inline void push(Element & e, bool isSource = false) {
		if(isSource && _prioritize_queued_sources) {
			_q_sources.insert_back(e);
		} else {
			_q_nonsources.insert_back(e);
		}
	}
	/*void push_priority(Element & e) {
		_q.insert_front(e);
	}*/
	void push_list(EventList & ll, FlowNode * source_flow_node = NULL);
	/*void push_list_priority(EventList & ll) {
		_q.insert_front_all(ll);
	}*/
	inline bool pop(Element & e) {
		return (!_prioritize_queued_sources || _q_sources.isEmpty() ? _pop(e,_q_nonsources) : _pop(e,_q_sources));
	}
	void log_snapshot();
private:
	inline bool _pop(Element & e, EventList & q) {
		EventListNode * n = q.first();
		bool res = n != NULL;
		if(res) {
			e = n->shared_content();
			q.remove(n);
		}
		return res;
	}
private:
	EventList _q_nonsources;
	EventList _q_sources;
	bool _prioritize_queued_sources;
};

};



#endif // _OFLUX_QUEUE
