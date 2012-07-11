#ifndef _OFLUX_QUEUE
#define _OFLUX_QUEUE
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
