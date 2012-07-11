#ifndef _OFLUX_MELDING_RUNTIME_H
#define _OFLUX_MELDING_RUNTIME_H
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

#include "OFluxRunTime.h"

namespace oflux {
namespace runtime {
namespace melding {

class RunTime : public classic::RunTime {
public:
	friend class RunTimeThread;
	friend class UnlockRunTime;
	RunTime(const RunTimeConfiguration & rtc)
                : classic::RunTime(rtc)
                {}
        virtual ~RunTime() {}
        virtual classic::RunTimeThread * new_RunTimeThread(oflux_thread_t tid = 0);
};

class RunTimeThread : public classic::RunTimeThread {
public:
	RunTimeThread(RunTime * rt, oflux_thread_t tid = 0)
                : classic::RunTimeThread(rt,tid)
                { _condition_context_switch = true; }
	virtual ~RunTimeThread() {}
	virtual int execute_detached(EventBaseSharedPtr & ev,
                int & detached_count_to_increment);
};

} // namespace melding
} // namespace runtime
} // namespace oflux

#endif // _OFLUX_MELDING_RUNTIME_H
