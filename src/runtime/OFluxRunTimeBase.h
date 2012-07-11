#ifndef _OFLUX_RUNTIME_BASE_H
#define _OFLUX_RUNTIME_BASE_H
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

#include "OFlux.h"
#include "OFluxRunTimeAbstractForShim.h"
#include <dlfcn.h>
#include <vector>
#include <string>

namespace oflux {

namespace flow {
class FunctionMaps;
class Flow;
class Node;
} // namespace flow

typedef void (*initShimFnType) (RunTimeAbstract *);
typedef void (*deinitShimFnType) ();

class RunTimeBase : public RunTimeAbstractForShim {
public:
        static initShimFnType initShim;
        static deinitShimFnType deinitShim;
        
	RunTimeBase(const RunTimeConfiguration & rtc)
                : _rtc(rtc)
		, _rtc_ref(rtc)
                , _running(false)
                , _load_flow_next(false)
                {}
        virtual ~RunTimeBase() {}
        /**
         * @brief schedule a new flow load at some point convenient
         */
        void soft_load_flow() { _load_flow_next = true; }
        /**
         * @brief ask the runtime to shut down -- start() function returns
         */
        void soft_kill() { _running = false; }
        virtual bool running() { return _running; }
        virtual void hard_kill() { soft_kill(); }
	virtual int atomics_style() const { return 1; }
        virtual void log_snapshot() = 0;
        virtual void log_snapshot_guard(const char * guardname) = 0;
        virtual void getPluginNames(std::vector<std::string> & result) = 0;
	virtual int thread_count() = 0;
	virtual const RunTimeConfiguration & config() const { return _rtc; }
protected:
	RunTimeConfiguration         _rtc;
	const RunTimeConfiguration & _rtc_ref;
        bool                         _running;
        bool                         _load_flow_next;
};

// list of runtime factory functions:
extern RunTimeBase * _create_classic_runtime(const RunTimeConfiguration &);
extern RunTimeBase * _create_melding_runtime(const RunTimeConfiguration &);
extern RunTimeAbstract * _create_lockfree_runtime(const RunTimeConfiguration &);

} // namespace oflux

#endif // _OFLUX_RUNTIME_BASE_H
