#ifndef _OFLUX_QUEUE
#define _OFLUX_QUEUE

/**
 * @file OFluxQueue.h
 * @author Mark Pichora
 * So far this run time only has a FIFO queue.
 */

#include "OFlux.h"
#include "boost/shared_ptr.hpp"
#include <deque>
#include <vector>

// queue for events (FIFO)

namespace oflux {

/**
 * @class Queue
 * @brief the run time's queue data structure
 */
class Queue {
public:
	typedef boost::shared_ptr<EventBase> Element;
	Queue() {}
	~Queue() { _q.clear(); }
	static const char * element_name(Element &);
	void push(Element & e);
	void push_priority(Element & e);
	void push_list(std::vector<Element> & vec);
	void push_list_priority(std::vector<Element> & vec);
	bool pop(Element & e);
	void log_snapshot();
private:
	std::deque<Element> _q;
};

};



#endif // _OFLUX_QUEUE
