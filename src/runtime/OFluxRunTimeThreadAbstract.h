#ifndef OFLUX_RUNTIME_THREAD_ABSTRACT_H
#define OFLUX_RUNTIME_THREAD_ABSTRACT_H

#include <vector>
#include "OFlux.h"

namespace oflux {

class RunTimeThreadAbstract {
public:
	virtual void submitEvents(const std::vector<EventBasePtr> &) = 0;

}; 

}// namespace oflux


#endif
