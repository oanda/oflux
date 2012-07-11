#ifndef OFLUX_RUNTIME_ABSTRACT_H
#define OFLUX_RUNTIME_ABSTRACT_H
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

#include <string>
#include <vector>
#include "OFlux.h"
#include "OFluxConfiguration.h"

namespace oflux {

namespace flow {
 class Flow;
 class FunctionMapsAbstract;
} // namespace flow

struct EnvironmentVar {
public:
	EnvironmentVar(int = 0);

	bool nostart;
	int  runtime_number;
};

class RunTimeThreadAbstract;

class RunTimeAbstract {
public:
	virtual ~RunTimeAbstract() {}

	virtual void start() = 0;

	virtual void soft_kill() = 0;

	virtual void hard_kill() = 0;

	virtual void soft_load_flow() = 0;

	virtual void log_snapshot() = 0;

	virtual void log_snapshot_guard(const char * guardname) = 0;

        virtual void getPluginNames(std::vector<std::string> & result) = 0;

	virtual int thread_count() = 0;
	virtual RunTimeThreadAbstract * thread() = 0;

	virtual flow::Flow * flow() = 0;

	virtual void submitEvents(const std::vector<EventBasePtr> & ) = 0;
	virtual const RunTimeConfiguration & config() const = 0;
};

namespace runtime {

class Factory {
public:
	enum {    classic  = 0
		, melding  = 1
		, lockfree = 4 
	};

	static RunTimeAbstract * create(int type, const RunTimeConfiguration & rtc);
};


} // namespace runtime

} // namespace oflux



#endif // OFLUX_RUNTIME_ABSTRACT_H
