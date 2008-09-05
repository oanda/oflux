#ifndef _O_FLUX
#define _O_FLUX
#include <boost/shared_ptr.hpp>

namespace oflux {

#define OFLUX_RUNTIME_VERSION "C++.1v0.2"

class LoggingAbstract;

extern LoggingAbstract * logger;

// compile-time assertions
template <bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};

// the empty node input type (internal detail)
struct _E {};

union E { 
	_E _e; 
};

extern E theE;

class EventBase;
class FlowNode;

typedef bool (*ConditionFn)(const void *);

typedef boost::shared_ptr<EventBase> (*CreateNodeFn)(boost::shared_ptr<EventBase>,FlowNode *);

typedef void (*GuardTransFn)(void *, const void *);

class AtomicMapAbstract;

struct CreateMap {
	const char * name;
	CreateNodeFn createfn;
};


struct ConditionalMap {
	const int unionnumber;
	const int argno;
	const char * name;
	ConditionFn condfn;
};

struct GuardTransMap {
	const char * guardname;
	const int union_number;
	const int argnum;
	const int * argnos;
	GuardTransFn guardtransfn;
};

struct AtomicMapMap {
	const char * guardname;
	AtomicMapAbstract * amap;
};

time_t fast_time(time_t * tloc); // fast time (from Paul)

}; // namespace


#endif
