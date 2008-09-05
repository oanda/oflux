#ifndef _OFLUX_QUEUE
#define _OFLUX_QUEUE

/**
 * @filename OFluxQueue.h
 * @author Mark Pichora
 * So far this run time only has a FIFO queue.
 */

#include "OFlux.h"
#include "boost/shared_ptr.hpp"
#include <deque>
#include <vector>
#ifdef HAS_DTRACE
#include "ofluxprobe.h"
#else
#define OFLUX_FIFO_PUSH(X)
#define OFLUX_FIFO_POP(X)
#endif

#define _FIFO_PUSH(X) OFLUX_FIFO_PUSH(const_cast<char *>(X))
#define _FIFO_POP(X)  OFLUX_FIFO_POP(const_cast<char *>(X))

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
	void push(Element & e) {
		_q.push_back(e);
		_FIFO_PUSH(element_name(e));
	}
	void push_priority(Element & e) {
		_q.push_front(e);
		_FIFO_PUSH(element_name(e));
	}
	void push_list(std::vector<Element> & vec) {
		for(int i = 0; i < (int)vec.size(); i++) {
			_q.push_back(vec[i]);
			_FIFO_PUSH(element_name(vec[i]));
		}
	}
	void push_list_priority(std::vector<Element> & vec) {
		// reverse
		for(int i = ((int)vec.size())-1; i >= 0; i--) {
			_q.push_front(vec[i]);
			_FIFO_PUSH(element_name(vec[i]));
		}
	}
	bool pop(Element & e) {
		bool res = _q.size() > 0;
		if(res) {
			e = _q.front();
			_q.pop_front();
			_FIFO_POP(element_name(e));
		}
		return res;
	}
	void log_snapshot();
private:
	std::deque<Element> _q;
};

};



#endif // _OFLUX_QUEUE
