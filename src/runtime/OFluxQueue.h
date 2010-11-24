#ifndef _OFLUX_QUEUE
#define _OFLUX_QUEUE

/**
 * @file OFluxQueue.h
 * @author Mark Pichora
 * So far this run time only has a FIFO queue.
 */

#include "OFlux.h"
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
	typedef EventBasePtr Element;
	Queue() {}
	~Queue() { _q.clear(); }
	static const char * element_name(const Element &);
	void push(Element & e);
	void push_priority(Element & e);
	void push_list(const std::vector<Element> & vec);
	void push_list_priority(const std::vector<Element> & vec);
	bool pop(Element & e);
	void log_snapshot();
private:
	std::deque<Element> _q;
};

};



#endif // _OFLUX_QUEUE
