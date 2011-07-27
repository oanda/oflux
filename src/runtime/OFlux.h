#ifndef _O_FLUX
#define _O_FLUX
#include "OFluxSharedPtr.h"
#ifdef Darwin
#include <boost/tr1/tr1/functional>
#else
#include <string>
#include <functional>
#include <tr1/functional_hash.h>
#endif


/**
 * @mainpage OFlux Runtime (and compiler)
 *   OFlux is an event-based datadriven runtime & compiler
 *   which extends C++.  It is meant to be used to control the flow
 *   of your (reactive service-oriented) program by managing threads
 *   for you.  Shared data is handled using guards which have various
 *   data structure semantics embedded into them (e.g singleton, map,
 *   pool).
 */

namespace oflux {

extern const char * runtime_version;

extern "C" void trigger_probe_aha_exception_begin_throw(int i);

#define OFLUX_RUNTIME_VERSION runtime_version

namespace logging {
class Abstract;
extern Abstract * logger;
} // namespace logging

// concept checking
template<typename T> struct IsPtr { enum { val = false }; };
template<typename R> struct IsPtr<R *> { enum { val = true }; };

// compile-time assertions
template <bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};

template<typename T>
class InsistOnIsPtr {
  static CompileTimeAssert<IsPtr<T>::val> type_must_be_ptr;
};

template<typename C>
struct hash {
        inline size_t operator()(const C & c) const
        {
                return std::tr1::hash<C>()(c);
        }
};

template<typename PC>
struct hash_ptr {
        inline size_t operator()(const PC * p) const
        {
                return hash<PC>()(*p);
        }
};

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

class AHAException { // AtomicsHolderAbstractException
public:
	AHAException(const int ind
		, const char * function = "") throw()
		: _ind(ind)
		, _func(function)
	{}

	int index() const { return _ind; }
	const char * func() const { return _func; }

private:
	int _ind;
	const char * _func;

};
		
namespace atomic {
class AtomicsHolderAbstract {
public:
	virtual void * getDataLexical(int i) = 0;

	template<typename T>
	T getDataLexicalThrowOnNull(int i) {
		void * vp = getDataLexical(i);
		if(vp == NULL) {
			trigger_probe_aha_exception_begin_throw(i);
			throw AHAException(i,__FUNCTION__);
		}
		return reinterpret_cast<T>(vp);
	}
	template<typename T>
	T getDataLexical(int i) {
		void * vp = getDataLexical(i);
		return reinterpret_cast<T>(vp);
	}
};
} // namespace atomic

	

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


// ----- EventBase Pointers (pretty important items really) ----

class EventBase;
typedef shared_ptr<EventBase> EventBaseSharedPtr;

/// implementation using shared pointers:
#ifdef SHARED_PTR_EVENTS
typedef EventBaseSharedPtr EventBasePtr; // full shared
#define mk_EventBasePtr(X) EventBasePtr(X)
#define mk_EventBaseSharedPtr(X) X
#define get_EventBasePtr(X) (X.get())
#define take_EventBasePtr(GETS,VAL) GETS.swap(VAL)
#define get_EventBaseSharedPtr(X) X
#define recover_EventBasePtr(X) (X.recover())
#define recover_when_not_shared_EventBasePtr(X) X
#define checked_recover_EventBasePtr(X) \
  { bool ev_recover_res = X.recover(); \
    assert(ev_recover_res && "event should not be NULL on fn recover failure"); }

#else  // SHARED_PTR_EVENTS
/// implementation that greatly limits the use of shared pointers
typedef EventBase * EventBasePtr; // non shared
#define mk_EventBasePtr(X) (X)
#define mk_EventBaseSharedPtr(X) EventBaseSharedPtr(X)
#define get_EventBasePtr(X) (X)
#define get_EventBaseSharedPtr(X) (X.get())
#define take_EventBasePtr(GETS,VAL) GETS = VAL
#define recover_EventBasePtr(X) X
#define checked_recover_EventBasePtr(X) 
#define recover_when_not_shared_EventBasePtr(X) (X.recover());

#endif // SHARED_PTR_EVENTS




namespace flow {
 class Node;
} //namespace flow

extern "C" {
typedef void InitFunction(void *);
typedef void DeInitFunction();
}

typedef bool (*ConditionFn)(const void *);

typedef EventBasePtr (*CreateNodeFn)(EventBaseSharedPtr,const void *, flow::Node *);

namespace doors {
  struct ServerDoorCookie;
  class ServerDoorCookieVirtualDestructor;
} // doors

class RunTimeAbstract;

typedef doors::ServerDoorCookieVirtualDestructor * (*CreateDoorFn)(const char *, const char *, RunTimeAbstract *);

typedef bool (*GuardTransFn)(void *, const void *, atomic::AtomicsHolderAbstract *);

namespace atomic {
 class AtomicMapAbstract;
} // namespace atomic

template< typename FP >
struct FunctionLookup {
	const char * name;
	FP createfn;
};

typedef FunctionLookup<CreateNodeFn> CreateMap;
typedef FunctionLookup<CreateDoorFn> CreateDoorMap;

template< typename FP >
struct ModularFunctionLookup {
	const char * scope;
	FunctionLookup<FP> * map;
};

typedef ModularFunctionLookup<CreateNodeFn> ModularCreateMap;
typedef ModularFunctionLookup<CreateDoorFn> ModularCreateDoorMap;

struct ConditionalMap {
	const char * unionhash;
	const int argno;
	const char * name;
	ConditionFn condfn;
};

struct GuardTransMap {
	const char * guardname;
	const char * unionhash;
        const char * hash;
        const int wtype;
        const bool late;
	GuardTransFn guardtransfn;
};

struct AtomicMapMap {
	const char * guardname;
	atomic::AtomicMapAbstract ** amap;
};

typedef void * (*FlatIOConversionFun)(const void *);

struct IOConverterMap {
        const char * from_unionhash;
        const char * to_unionhash;
        FlatIOConversionFun conversion_fun;
	// rest is for troubleshooting
	const char * to_str_name;
	const char * from_str_name;
	const char * file;
	int line;
};


time_t fast_time(time_t * tloc); // fast time (from Paul)

}; // namespace


#endif
