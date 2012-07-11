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
#include "flow/OFluxFlowGuard.h"
#include "OFluxLogging.h"
#include <vector>


namespace oflux {
namespace flow {

void
GuardReference::log_aha_warning(const char *s, int arg)
{
	oflux_log_warn("oflux::flow::GuardReference::get() threw "
		"an AHAException -- so an earlier guard "
		"was not aquired %s, guard arg %d\n"
		, s
		, arg);
}

void
Guard::drain()
{
        std::vector<EventBasePtr> rel_vector;
        shared_ptr<atomic::AtomicMapWalker> walker(_amap->walker());
        const void * v = NULL;
        atomic::Atomic * a = NULL;
	EventBasePtr nullPtr(NULL); 
		// don't have the original events
		// Atomic::release needs to handle by_ev == NULL
        while(walker->next(v,a)) {
                a->release(rel_vector,nullPtr);
        }
}

} // namespace flow
} // namespace oflux
