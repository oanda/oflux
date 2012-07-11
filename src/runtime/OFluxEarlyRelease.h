#ifndef OFLUX_EARLY_RELEASE
#define OFLUX_EARLY_RELEASE
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

//
// Normally the guards of a node are held for the duration of the node function execution.
// This interface allows you to release them earlier than that (which can be good to reduce contention
// (particularly in a lock-free setting)

namespace oflux {

class RunTimeAbstract;

void release_all_guards(RunTimeAbstract * rt); 
	// must be called within the dynamic scope of a node function
	// not within the runtime proper!

} // namespace oflux

#endif // OFLUX_EARLY_RELEASE
