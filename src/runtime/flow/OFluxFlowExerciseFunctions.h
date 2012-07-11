#ifndef OFLUX_FLOW_EXERCISE_FUNCTIONS
#define OFLUX_FLOW_EXERCISE_FUNCTIONS
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

#include "flow/OFluxFlowFunctions.h"

namespace oflux {
namespace atomic {
 class AtomicMapAbstract;
 class Atomic;
} // namespace atomic
namespace flow {

 class Flow;

namespace exercise {

typedef void (*ReleaseGuardsFn)();

extern ReleaseGuardsFn release_guards;

extern long max_nsec_wait;

void node_report(oflux::flow::Flow *);

class AtomicAbstract {
public:
	struct P {
		const char * name;
		oflux::atomic::Atomic * atomic;
	};
	AtomicAbstract() {}
	virtual ~AtomicAbstract() {}
	virtual void set_wtype(int wtype) = 0;
	virtual oflux::atomic::AtomicMapAbstract * atomic_map() = 0;
	virtual void report(const char * name,bool) = 0;
};

class AtomicSetAbstract {
public:
	virtual AtomicAbstract & get(const char * guardname) = 0;
	virtual void report() = 0;
	virtual size_t size() const = 0;
	virtual void fill(AtomicAbstract::P arr[]) = 0;
};

} // namespace exercise
	

class ExerciseFunctionMaps : public FunctionMapsAbstract {
public:
	ExerciseFunctionMaps(int atomics_style);
	virtual Library* libraryFactory(const char * dir, const char * name);
        virtual CreateNodeFn lookup_node_function(const char * n) const;
        virtual CreateDoorFn lookup_door_function(const char * n) const;
        virtual ConditionFn lookup_conditional(
		  const char * n
		, int argno
		, const char * unionhash) const;
        virtual GuardTransFn lookup_guard_translator(
		  const char * guardname
                , const char * unionhash
                , const char * hash
                , int wtype
		, bool late) const;
        virtual oflux::atomic::AtomicMapAbstract * lookup_atomic_map(
		const char * guardname) const;

        virtual FlatIOConversionFun lookup_io_conversion(
		  const char * from_unionhash
		, const char * to_unionhash) const;
	exercise::AtomicSetAbstract * atomic_set() { return _atomic_set; }
private:
	exercise::AtomicSetAbstract * _atomic_set;
};

} // namespace flow
} // namespace oflux



#endif // OFLUX_FLOW_EXERCISE_FUNCTIONS

