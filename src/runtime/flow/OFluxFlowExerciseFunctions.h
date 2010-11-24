#ifndef OFLUX_FLOW_EXERCISE_FUNCTIONS
#define OFLUX_FLOW_EXERCISE_FUNCTIONS

#include "flow/OFluxFlowFunctions.h"

namespace oflux {
namespace atomic {
 class AtomicMapAbstract;
} // namespace atomic
namespace flow {

namespace exercise {

class AtomicAbstract {
public:
	AtomicAbstract() {}
	virtual ~AtomicAbstract() {}
	virtual void set_wtype(int wtype) = 0;
	virtual oflux::atomic::AtomicMapAbstract * atomic_map() = 0;
};

class AtomicSetAbstract {
public:
	virtual AtomicAbstract & get(const char * guardname) = 0;
};

} // namespace exercise
	

class ExerciseFunctionMaps : public FunctionMapsAbstract {
public:
	ExerciseFunctionMaps(int atomics_style);
	virtual Library* libraryFactory(const char * dir, const char * name);
        virtual CreateNodeFn lookup_node_function(const char * n) const;
        virtual ConditionFn lookup_conditional(const char * n, int argno, int unionnumber) const;
        virtual GuardTransFn lookup_guard_translator(
		  const char * guardname
                , int union_number
                , const char * hash
                , int wtype
		, bool late) const;
        virtual oflux::atomic::AtomicMapAbstract * lookup_atomic_map(const char * guardname) const;

        virtual FlatIOConversionFun lookup_io_conversion(int from_unionnumber, int to_unionnumber) const;

private:
	exercise::AtomicSetAbstract * _atomic_set;
};

} // namespace flow
} // namespace oflux



#endif // OFLUX_FLOW_EXERCISE_FUNCTIONS

