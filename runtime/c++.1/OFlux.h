#ifndef _O_FLUX
#define _O_FLUX
#include <boost/shared_ptr.hpp>

namespace oflux {

#define OFLUX_RUNTIME_VERSION "C++.1v0.24"

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

struct BaseOutputStructBase {
	void * __next; // NULL this on your own you
};

template<typename T>
struct BaseOutputStruct : public BaseOutputStructBase {
	inline T * push_new() 
		{ 
			T * t = new T();
			__next = t;
			t->__next = NULL; 
			return t;
		}
	inline T * next() const { return reinterpret_cast<T *>(__next); }
	inline void next(T * n) { __next = n; }
};

class OutputWalker {
public:
	OutputWalker(BaseOutputStructBase * bosb)
		: _bosb(bosb)
		{}
	void * next() 
		{
			BaseOutputStructBase * res = _bosb;
			if(res) _bosb = reinterpret_cast<BaseOutputStructBase *>(_bosb->__next);
			return res;
		}
private:
	BaseOutputStructBase * _bosb;
};

/** 
 * @brief template for tool to automatically push output lazily
 *
 * Performs a push_new() only on the first -> after a next()
 *
 * Use case:
 *
 *   PushTool<S_out> pt(out);
 *   for (int i = 0; i < 3; ++i) {
 *     pt->field_1 = i; // pushes new only on 2nd pass
 *     pt->field_2 = 2*i;
 *     pt.next();
 *   }
 *   return 0;
 */
template <typename OC>
class PushTool {
public:
    PushTool(OC *out): out_(out), pending_(false) {}
    OC *operator ->() {
        if (pending_) {
            pending_ = false;
            return out_ = out_->push_new();
        } else {
            return out_;
        }
    }
    void next(void) { pending_ = true; }

private:
    OC *out_;
    bool pending_;
};

extern E theE;

class EventBase;
class FlowNode;

typedef bool (*ConditionFn)(const void *);

typedef boost::shared_ptr<EventBase> (*CreateNodeFn)(boost::shared_ptr<EventBase>,void *, FlowNode *);

typedef void (*GuardTransFn)(void *, const void *);

class AtomicMapAbstract;

struct CreateMap {
	const char * name;
	CreateNodeFn createfn;
};

struct ModularCreateMap {
	const char * scope;
	CreateMap * map;
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
