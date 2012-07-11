#ifndef OFLUX_DISTRIBUTED_COUNTER_H
#define OFLUX_DISTRIBUTED_COUNTER_H
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
 * @file OFluxDistributedCounter.h
 * @author Mark Pichora
 * A distributed counter is an object with per-thread local counters
 * whose sum represents the overall value of the counter.  This is done
 * so that each thread can quickly increment the counter, and not pay
 * any contention penalty on the changes.
 */

#include <cstdlib>
#include "lockfree/OFluxThreadNumber.h"
#include "lockfree/OFluxMachineSpecific.h"


namespace oflux {
namespace lockfree {

template< typename CT
	, size_t num_threads=MachineSpecific::Max_Threads_Liberal>
class Counter {
public:
	Counter()
	{
		for(size_t i = 0; i < num_threads; ++i) {
			_v[i] = 0;
		}
	}
	Counter(const CT & ct)
	{
		for(size_t i = 0; i < num_threads; ++i) {
			_v[i] = 0;
		}
		_v[0] = ct;
	}	
	CT operator+=(CT by) { // add by
		return _v[_tn.index] += by;
	}
	CT operator-=(CT by) { // subtract by
		return _v[_tn.index] -= by;
	}
	CT operator++(int) { // pre inc
		return ++_v[_tn.index];
	}
	CT operator++() { // post inc
		return _v[_tn.index]++;
	}
	CT operator--(int) { // pre dec
		return --_v[_tn.index];
	}
	CT operator--() { // post dec
		return _v[_tn.index]--;
	}
	volatile CT & operator[](size_t i) {
		return _v[i];
	}
	CT value() const
	{
		CT res = 0;
		for(size_t i = 0; i < std::min(ThreadNumber::num_threads,num_threads); ++i) {
			res += _v[i];
		}
		return res;
	}
private:
	volatile CT _v[num_threads];
};

} // namespace lockfree
} // namespace oflux

#endif
