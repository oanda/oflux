#include "flow/OFluxFlowExerciseFunctions.h"
#include "flow/OFluxFlowLibrary.h"
#include "flow/OFluxFlowNode.h"
#include "flow/OFluxFlow.h"
#include "event/OFluxEvent.h"
#include "atomic/OFluxAtomicHolder.h"
#include "atomic/OFluxAtomicInit.h"
#include "lockfree/atomic/OFluxLFAtomic.h"
#include "lockfree/atomic/OFluxLFAtomicPooled.h"
#include "lockfree/atomic/OFluxLFAtomicReadWrite.h"
#include "lockfree/OFluxDistributedCounter.h"
#include "OFluxEarlyRelease.h"
#include "OFluxThreads.h"
#include "OFluxLogging.h"
#include "OFluxRollingLog.h"
#include <cassert>
#include <string>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <ctime>
#include <cstdio>

namespace oflux {
namespace atomic {
namespace instrumented {

template<typename D> // D is POD
class LocalRollingLog : public oflux::RollingLog<D> {
public:
	LocalRollingLog()
	{}
	void report()
	{
#define REPORT_BUFFER_LENGTH 1024
		char buff[REPORT_BUFFER_LENGTH];
		long long s_index = (this->index+1);
		oflux_thread_t local_tid = oflux_self();
		oflux_log_info(PTHREAD_PRINTF_FORMAT " log tail %p:\n", local_tid, this);
		size_t zero_repeat = 0;
		for(size_t i=0; i < oflux::RollingLog<D>::log_size; ++i) {
#define DO_zero_repeat_OUTPUT \
 if(zero_repeat) { \
   oflux_log_info(PTHREAD_PRINTF_FORMAT " 0] (repeats %u times)\n",local_tid,zero_repeat); \
   zero_repeat = 0; \
   }
			if(this->log[(i+s_index)%oflux::RollingLog<D>::log_size].index) {
				DO_zero_repeat_OUTPUT
				this->log[(i+s_index)%oflux::RollingLog<D>::log_size].d.report(buff);
				oflux_log_info(PTHREAD_PRINTF_FORMAT " %lld] %s\n"
					, local_tid
					, this->log[(i+s_index)%oflux::RollingLog<D>::log_size].index
					, buff);
			} else {
				++zero_repeat;
			}
		}
		DO_zero_repeat_OUTPUT
	}
};


class AtomicAbstract : public oflux::atomic::Atomic {
public:
	virtual void report() = 0;
};

// husk for an Atomic that lets me see what its doing
template<typename A>
class Atomic : public oflux::atomic::Atomic {
public:
	struct Observation {
		char name;
		int wtype;
		EventBase * evptr;
		EventBase * revptr;
		const char * fname;
		long long term_index;
		int res;
		oflux_thread_t tid;

		void report(char * buff)
		{
			snprintf(buff, 1000, " %c %d %-8p %-8p %lld %d " PTHREAD_PRINTF_FORMAT " %s"
				, name
				, wtype
				, evptr
				, revptr
				, term_index
				, res
				, tid
				, fname ? fname : "<null>"
				);
		}
	};

	LocalRollingLog<Observation> log;
	long long last_a;
	long long last_r;

	virtual void report() 
	{ 
		if(log.at()) { log.report(); }
		oflux_log_info("last a @ %lld, last r @ %lld\n"
			, last_a
			, last_r);
	}

	Atomic(void * data)
		: last_a(0)
		, last_r(0)
		, _a(data)
	{}
	virtual ~Atomic() {}
	virtual void ** data() { return _a.data(); }
	virtual int held() const { return _a.held(); }
	virtual void release(
		  std::vector<EventBasePtr> & rel_ev
		, EventBasePtr & by_ev)
	{
		Observation & o = log.submit();
		last_r = log.at();
		o.wtype = 0;
		o.term_index = 0;
		o.name = 'R';
		o.evptr = by_ev.get();
		o.revptr = 0;
		o.fname = (by_ev->flow_node() ? by_ev->flow_node()->getName() : NULL);
		o.tid = oflux_self();
		size_t pre_sz = rel_ev.size();
		_a.release(rel_ev,by_ev);
		o.name = 'r';
		o.term_index = log.at();
		o.res = rel_ev.size()-pre_sz;
		if(o.res) {
			o.revptr = rel_ev[pre_sz].get();
		}
	}
	virtual bool acquire_or_wait(
		  EventBasePtr & ev
		, int wtype)
	{
		Observation & o = log.submit();
		last_a = log.at();
		o.wtype = wtype;
		o.term_index = 0;
		o.name = 'A';
		o.revptr = 0;
		o.evptr = ev.get();
		o.fname = (ev->flow_node() ? ev->flow_node()->getName() : NULL);
		o.tid = oflux_self();
		bool res = _a.acquire_or_wait(ev,wtype);
		o.name = 'a';
		o.term_index = log.at();
		o.res = res;
		return res;
	}
	virtual size_t waiter_count()
	{
		return _a.waiter_count();
	}
	virtual bool has_no_waiters()
	{
		return _a.has_no_waiters();
	}
	virtual void relinquish() { _a.relinquish(); }
	virtual int wtype() const 
	{
		return _a.wtype();
	}
	virtual const char * atomic_class() const
	{
		static bool done = false;
		static char ac_name[1024] = 
			"ins:                       " 
			"                           "
			"                           ";
		if(!done) {
			done = true;
			strcpy(ac_name+5,_a.atomic_class());
		}
		return ac_name;
	}
	virtual void log_snapshot_waiters() const
	{
		_a.log_snapshot_waiters();
	}
	virtual bool is_pool_like() const
	{
		return _a.is_pool_like();
	}
private:
	A _a;
}; // Atomic (instrumented template)
	

} // namespace instrumented
} // namespace atomic
namespace flow {

namespace exercise {

ReleaseGuardsFn release_guards = NULL;

#define MAX_NODES 1024

//oflux::lockfree::Counter<long long> node_creations[MAX_NODES];
//oflux::lockfree::Counter<long long> node_executions[MAX_NODES];
oflux::flow::Node * nodes[MAX_NODES];

void
node_report(oflux::flow::Flow *)
{
	oflux_log_info("Node report:\n");
	for(size_t i = 0; i < MAX_NODES; ++i) {
		if(nodes[i]) {
			oflux_log_info("  %s %lld %lld\n"
				, nodes[i]->getName()
				, nodes[i]->instances()
				, nodes[i]->executions());
		}
	}
}

template< typename AB, const size_t max_size >
class ManyThings {
public:
	ManyThings()
	{ 
		::bzero(buffer,sizeof(buffer)); 
	}

	template< typename C >
	void overwrite(const C & c)
	{
		::bzero(buffer,sizeof(buffer)); 
		::memcpy(buffer,&c,sizeof(C));
	}
	inline AB * get() { return reinterpret_cast<AB *>(buffer); }
private:
	char buffer[max_size];
};


static void
pool_init(oflux::atomic::AtomicMapAbstract * ama)
{
	oflux::atomic::GuardInserter populator(ama);
	// just one item for now
	size_t pool_size = 1;
	char * num_items = getenv("EXERCISE_POOLSIZE");
	if(num_items) {
		pool_size = atoi(num_items);
	}
	oflux_log_info("pool_init sizes to %u items\n", pool_size);
	for(size_t i = 0; i < pool_size; ++i) {
		populator.insert(NULL,new int(0));
	}
}

class LFAtomic : public AtomicAbstract {
public:
#ifdef ATOM_INSTRUMENTATION
	typedef oflux::atomic::instrumented::Atomic<oflux::lockfree::atomic::AtomicExclusive> AtomicEx;
#else
	typedef oflux::lockfree::atomic::AtomicExclusive AtomicEx;
#endif
	typedef oflux::atomic::AtomicMapTrivial<AtomicEx> AtomicExclusive;
#ifdef ATOM_INSTRUMENTATION
	typedef oflux::atomic::instrumented::Atomic<oflux::lockfree::atomic::AtomicReadWrite> AtomicRW;
#else
	typedef oflux::lockfree::atomic::AtomicReadWrite AtomicRW;
#endif
	typedef oflux::atomic::AtomicMapTrivial<AtomicRW> AtomicReadWrite;
	typedef oflux::atomic::AtomicMapTrivial<oflux::lockfree::atomic::AtomicFree> AtomicFree;
	typedef oflux::lockfree::atomic::AtomicPool AtomicPool;

#define MAX(X,Y) ((X)>(Y) ? X : Y)

	enum { max_size = MAX( sizeof(AtomicExclusive)
			, MAX( sizeof(AtomicReadWrite)
			, MAX( sizeof(AtomicFree)
			,      sizeof(AtomicPool) ) ) ) };
	
	LFAtomic()
		: _exclusive()
		, _readwrite()
		, _free()
		, _pool()
	{ pool_init(&_pool); }
	virtual void set_wtype(int wtype);
	virtual oflux::atomic::AtomicMapAbstract * atomic_map()
	{ return _many.get(); }
	virtual void report(const char *, bool);
private:
	ManyThings<oflux::atomic::AtomicMapAbstract, max_size> _many;
	AtomicExclusive _exclusive;
	AtomicReadWrite _readwrite;
	AtomicFree _free;
	AtomicPool _pool;
};

static void atomic_report(const char * name, atomic::AtomicMapAbstract *ama, bool full)
{
	int * null_check_vptr = reinterpret_cast<int *>(ama);
	if(!null_check_vptr || !*null_check_vptr) {
		oflux_log_info("  %s unused\n", name);
		return;
	}
	atomic::AtomicMapWalker * amw = ama->walker();
	const void * k = NULL;
	atomic::Atomic * a = NULL;
	while(amw->next(k,a)) {
		const char * at_class = a->atomic_class();
		oflux_log_info("  %s : %s %s %u %d\n"
			, name
			, at_class
			, (a->held() ? "held" : "free")
			, a->waiter_count()
			, a->wtype());
		if(at_class[0] == 'i' && full) {
			oflux::atomic::instrumented::AtomicAbstract * aa =
				reinterpret_cast<oflux::atomic::instrumented::AtomicAbstract *>(a);
			aa->report();
		}
	}
}

void
LFAtomic::report(const char * name, bool full)
{
	atomic::AtomicMapAbstract * ama = _many.get();
	atomic_report(name,ama,full);
}

void 
LFAtomic::set_wtype(int wtype)
{
	if(wtype == oflux::atomic::AtomicReadWrite::Read
			|| wtype == oflux::atomic::AtomicReadWrite::Write
			|| wtype == oflux::atomic::AtomicReadWrite::Upgradeable) {
		_many.overwrite(_readwrite);
	} else if(wtype == oflux::atomic::AtomicExclusive::Exclusive) {
		_many.overwrite(_exclusive);
	} else if(wtype == oflux::atomic::AtomicFree::Free) {
		_many.overwrite(_free);
	} else if(wtype == oflux::atomic::AtomicPooled::Pool) {
		_many.overwrite(_pool);
	} else {
		assert(0 && "expected a valid wtype");
	}
}

class ClAtomic : public AtomicAbstract {
public:
#ifdef ATOM_INSTRUMENTATION
	typedef oflux::atomic::instrumented::Atomic<oflux::atomic::AtomicExclusive> AtomicEx;
#else
	typedef oflux::atomic::AtomicExclusive AtomicEx;
#endif
	typedef oflux::atomic::AtomicMapTrivial<AtomicEx> AtomicExclusive;
#ifdef ATOM_INSTRUMENTATION
	typedef oflux::atomic::instrumented::Atomic<oflux::atomic::AtomicReadWrite> AtomicRW;
#else
	typedef oflux::atomic::AtomicReadWrite AtomicRW;
#endif
	typedef oflux::atomic::AtomicMapTrivial<AtomicRW > AtomicReadWrite;
	typedef oflux::atomic::AtomicMapTrivial<oflux::atomic::AtomicFree> AtomicFree;
	typedef oflux::atomic::AtomicPool AtomicPool;

	enum { max_size = MAX( sizeof(AtomicExclusive)
			, MAX( sizeof(AtomicReadWrite)
			, MAX( sizeof(AtomicFree)
			,      sizeof(AtomicPool) ) ) ) };
	ClAtomic()
		: _exclusive()
		, _readwrite()
		, _free()
		, _pool()
	{ pool_init(&_pool); }
	virtual void set_wtype(int wtype);
	virtual oflux::atomic::AtomicMapAbstract * atomic_map()
	{ return _many.get(); }
	virtual void report(const char *,bool);
private:
	ManyThings<oflux::atomic::AtomicMapAbstract, max_size> _many;
	AtomicExclusive _exclusive;
	AtomicReadWrite _readwrite;
	AtomicFree _free;
	AtomicPool _pool;
};

void
ClAtomic::report(const char * name, bool full)
{
	atomic::AtomicMapAbstract * ama = _many.get();
	atomic_report(name,ama,full);
}

void 
ClAtomic::set_wtype(int wtype)
{
	if(wtype == oflux::atomic::AtomicReadWrite::Read
			|| wtype == oflux::atomic::AtomicReadWrite::Write
			|| wtype == oflux::atomic::AtomicReadWrite::Upgradeable) {
		_many.overwrite(_readwrite);
	} else if(wtype == oflux::atomic::AtomicExclusive::Exclusive) {
		_many.overwrite(_exclusive);
	} else if(wtype == oflux::atomic::AtomicFree::Free) {
		_many.overwrite(_free);
	} else if(wtype == oflux::atomic::AtomicPooled::Pool) {
		_many.overwrite(_pool);
	} else {
		assert(0 && "expected a valid wtype");
	}
}

class AtomicSet : public AtomicSetAbstract {
public:

	enum { Lockfree = 1, Classic = 0 };
	
	AtomicSet(int guardstyle) : style(guardstyle) {}
	virtual AtomicAbstract & get(const char * guardname);
	virtual void report();
	virtual size_t size() const { return _map.size(); }
	virtual void fill(AtomicAbstract::P arr[]);
private:
	int style;
	std::map<std::string,AtomicAbstract *> _map;
};

void
AtomicSet::fill(AtomicAbstract::P arr[])
{
	std::map<std::string,AtomicAbstract *>::iterator itr = _map.begin();
	std::map<std::string,AtomicAbstract *>::iterator itr_end = _map.end();
	size_t i =0;
	while(itr != itr_end) {
		arr[i].name = (*itr).first.c_str();
		int k = 0;
		arr[i].atomic = NULL;
		oflux::atomic::AtomicMapAbstract * amap = (*itr).second->atomic_map();
		if(amap && *(reinterpret_cast<int *>(amap))) {
			amap->get(arr[i].atomic,&k);
		}
		++i;
		++itr;
	}
	arr[i].name = 0;
}


void
AtomicSet::report()
{
	oflux_log_info("AtomicSet report:\n");
	char * watch_atomic = getenv("EXERCISE_WATCH");
	std::map<std::string,AtomicAbstract *>::iterator itr = _map.end();
	char * setofwatched[20];
	setofwatched[0] = 0;
	char buff[4096*4];
	if(watch_atomic) {
		strncpy(buff,watch_atomic,sizeof(buff));
		char * lasts = NULL;
		const char * delim = ",";
		size_t i = 0;
		for(char * s = strtok_r(buff,delim,&lasts)
			; s && (i < sizeof(setofwatched)/sizeof(char*) -1)
			; s=strtok_r(NULL,delim,&lasts)) {
			setofwatched[i] = s;
			oflux_log_info("exercise watching %s.\n",s);
			++i;
		}
		setofwatched[i] = 0;
	}
	for(itr = _map.begin(); itr != _map.end(); ++itr) {
		bool fd = false;
		for(size_t i = 0; setofwatched[i]; ++i) {
			if(strcmp(setofwatched[i],itr->first.c_str()) == 0) {
				fd = true;
			}
		}
		
		itr->second->report(itr->first.c_str(), fd);
	}
}

AtomicAbstract & 
AtomicSet::get(const char * guardname)
{
	std::string str = guardname;
	if(_map.find(str) == _map.end()) {
		_map[str] = (style 
			? static_cast<AtomicAbstract *>(new LFAtomic())
			: static_cast<AtomicAbstract *>(new ClAtomic())
			);
	}
	AtomicAbstract * ama = _map[str];
	assert(ama);
	return *ama;
}

long max_nsec_wait = 1000;

} // namespace exercise

class ExerciseLibrary : public Library {
public:
	ExerciseLibrary(const char * name, FunctionMapsAbstract * fms) 
		: Library(".",name)
	{ /* static */ _fms = fms; }
	virtual ~ExerciseLibrary() {}
	virtual bool load(int) { return true; }
protected:
	static FunctionMapsAbstract * getFMS(int) 
	{
		return _fms;
	}
        virtual void * _getSymbol(const char * name, bool ignoreError)
	{
		if(strncmp("flowfunctionmaps_",name,17) == 0) {
			// this is kinda a hack through
			// _fms which is a global
			// (give me closures in C++)
			return reinterpret_cast<void *>(getFMS);
		}
		return NULL; // failure
	}
private:
	static FunctionMapsAbstract * _fms;
};

FunctionMapsAbstract * ExerciseLibrary::_fms = NULL;

struct SingleAtom {
	int wtype;
	void * * data;
};

struct ExerciseEventDetail {
	struct Out_ : BaseOutputStruct<Out_> {
		typedef Out_ base_type;

		int value;
	};
	typedef Out_ In_;
	struct Atoms_ {
		Atoms_() : number(0) {}

		void fill(oflux::atomic::AtomicsHolder *);
		void report();

		const char * node_name;
		int node_id;
		SingleAtom atoms[MAX_ATOMICS_PER_NODE];
		int number;
	}; // Atoms_
	typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *);
	static nfunctype nfunc;
};

void
ExerciseEventDetail::Atoms_::report()
{
	static const char * wtype_strs[] =
		{ "     "
		, "Read "
		, "Write"
		, "Exclu"
		, "Upgda"
		, "Pool "
		, "Free "
		, "     "
		, "     "
		, "     "
		, "     "
		, "     "
		};
		
	for(int i = 0; i < number; ++i) {
		oflux_log_trace(" atom[%-2d] %s %p\n"
			, i
			, wtype_strs[atoms[i].wtype]
			, (atoms[i].data ? *(atoms[i].data) : 0));
	}
}

struct ExerciseSourceEventDetail : public ExerciseEventDetail {
	static nfunctype nfunc;
};

struct ExerciseErrorEventDetail : public ExerciseEventDetail {
	typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *, int);
	static nfunctype nfunc;
};

void
ExerciseEventDetail::Atoms_::fill(oflux::atomic::AtomicsHolder *ah)
{
	static int dontcare = 0;
	number = std::min(MAX_ATOMICS_PER_NODE,ah->number());
	for(int i = 0; i < number; ++i) {
		oflux::atomic::Atomic * atomic = ah->get(i,oflux::atomic::AtomicsHolder::HA_get_lexical)->atomic();
		atoms[i].data = atomic->data();
		if(!*(atoms[i].data)) {
			*(reinterpret_cast<int * *>(atoms[i].data)) = &dontcare;
		}
		atoms[i].wtype = atomic->wtype();
	}
}

int
exercise_node_function(
	  const ExerciseEventDetail::In_ * in
	, ExerciseEventDetail::Out_ * out
	, ExerciseEventDetail::Atoms_ * atoms)
{
	oflux_log_debug("[" PTHREAD_PRINTF_FORMAT "] exercise_node_function %s %d\n"
		, oflux_self()
		, atoms->node_name
		, in->value);
	atoms->report();
	out->value = in->value;
	//oflux::flow::exercise::node_executions[atoms->node_id%MAX_NODES]++;
	return 0;
}

int
exercise_error_node_function(
	  const ExerciseEventDetail::In_ * in
	, ExerciseEventDetail::Out_ *
	, ExerciseEventDetail::Atoms_ * atoms
	, int)
{
	oflux_log_debug("[" PTHREAD_PRINTF_FORMAT "]exercise_error_node_function %s %d\n"
		, oflux_self()
		, atoms->node_name
		, in->value);
	atoms->report();
	//oflux::flow::exercise::node_executions[atoms->node_id%MAX_NODES]++;
	return 0;
}

#define WAIT_A_LITTLE(L) \
	{ \
	timespec twait = {0 /*sec*/, L /*nsec*/}; \
	timespec trem; \
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] WAIT_A_LITTLE start %ld.%ld\n", oflux_self(), twait.tv_sec, twait.tv_nsec); \
	nanosleep(&twait,&trem); \
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] WAIT_A_LITTLE done \n", oflux_self()); \
	}

int
exercise_source_node_function(
	  const ExerciseEventDetail::In_ *
	, ExerciseEventDetail::Out_ * out
	, ExerciseEventDetail::Atoms_ * atoms)
{
	static unsigned seed = 334;
	out->value = rand_r(&seed);
	oflux_log_debug("[" PTHREAD_PRINTF_FORMAT "] exercise_source_node_function %s %d\n"
		, oflux_self()
		, atoms->node_name
		, out->value);
	atoms->report();
	if(exercise::release_guards) {
		(*exercise::release_guards)();
	}
#define MAX_NSEC_WAIT oflux::flow::exercise::max_nsec_wait
	WAIT_A_LITTLE((out->value)%MAX_NSEC_WAIT);
	//oflux::flow::exercise::node_executions[atoms->node_id%MAX_NODES]++;
	return 0;
}

ExerciseEventDetail::nfunctype ExerciseEventDetail::nfunc = exercise_node_function;
ExerciseEventDetail::nfunctype ExerciseSourceEventDetail::nfunc = exercise_source_node_function;
ExerciseErrorEventDetail::nfunctype ExerciseErrorEventDetail::nfunc = exercise_error_node_function;

ExerciseFunctionMaps::ExerciseFunctionMaps(int atomics_style)
	: _atomic_set(new exercise::AtomicSet(atomics_style))
{
}

Library *
ExerciseFunctionMaps::libraryFactory(const char * dir, const char * name)
{
	return new ExerciseLibrary(name,this);
}

} // namespace flow

#ifndef OFLUX_CONVERT_GENERAL
#define OFLUX_CONVERT_GENERAL
template<typename H>
inline H * convert(typename H::base_type *)
{
    enum { general_conversion_not_supported_bug = 0 } _an_enum;
    oflux::CompileTimeAssert<general_conversion_not_supported_bug> cta;
}

template<typename H>
inline H const * const_convert(typename H::base_type const * g)
{
    return convert<H>(const_cast<typename H::base_type *>(g));
}
#endif //OFLUX_CONVERT_GENERAL

template<>
inline flow::ExerciseEventDetail::Out_ * 
convert<flow::ExerciseEventDetail::Out_ >( flow::ExerciseEventDetail::Out_::base_type * s)
{
    return s;
}


namespace flow {

static bool
check_guard_duplication(flow::Node * fn)
{
	bool res = false;
	std::vector<GuardReference *> grefs = fn->guards();
	for(size_t i = 0; i < grefs.size(); ++i) {
		for(size_t j = i+1; j < grefs.size(); ++j) {
			if(grefs[i]->magic_number() == grefs[j]->magic_number()) {
				res = true;
			}
		}
	}
	return res;
}


EventBasePtr
create_special( 
	  EventBasePtr pred_node_ptr
        , const void * im_io_convert
        , flow::Node *fn)
{
	//exercise::node_creations[fn->id()%MAX_NODES]++;
	exercise::nodes[fn->id()] = fn;
	assert(!check_guard_duplication(fn));
	if(fn->getIsSource()) {
		// special case for sources
		EventBaseTyped<ExerciseSourceEventDetail> * ebt = 
			new Event<ExerciseSourceEventDetail>(
			  pred_node_ptr
			, reinterpret_cast<const IOConversionBase<ExerciseSourceEventDetail::In_> *>(im_io_convert)
			, fn);
		ebt->atomics_argument()->node_name = fn->getName();
		ebt->atomics_argument()->node_id = fn->id();
		return EventBasePtr(ebt);
	} else if(fn->getIsErrorHandler()) {
		EventBaseTyped<ExerciseErrorEventDetail> * ebt = 
			new ErrorEvent<ExerciseErrorEventDetail>(
			  pred_node_ptr
			, reinterpret_cast<const IOConversionBase<ExerciseErrorEventDetail::In_> *>(im_io_convert)
			, fn);
		ebt->atomics_argument()->node_name = fn->getName();
		ebt->atomics_argument()->node_id = fn->id();
		return EventBasePtr(ebt);
	} else {
		EventBaseTyped<ExerciseEventDetail> * ebt = 
			new Event<ExerciseEventDetail>(
			  pred_node_ptr
			, reinterpret_cast<const IOConversionBase<ExerciseEventDetail::In_> *>(im_io_convert)
			, fn);
		ebt->atomics_argument()->node_name = fn->getName();
		ebt->atomics_argument()->node_id = fn->id();
		return EventBasePtr(ebt);
	}
}


CreateNodeFn
ExerciseFunctionMaps::lookup_node_function(const char * ) const
{
	return create_special;
}

CreateDoorFn
ExerciseFunctionMaps::lookup_door_function(const char * ) const
{
	assert(0 && "door support incomplete for the exerciser"); // TODO
	return NULL;
}

bool 
exercise_condition_function(const void *v)
{
	const ExerciseEventDetail::Out_ * out =
		reinterpret_cast<const ExerciseEventDetail::Out_ *>(v);
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "]  exercise_condition_function %d\n"
		, oflux_self()
		, out->value);
	bool res = (out->value) % 2;
	return res;
}

ConditionFn 
ExerciseFunctionMaps::lookup_conditional(
	  const char * n
	, int argno
	, const char * unionhash) const
{
	return exercise_condition_function;
}

bool 
exercise_guard_trans_function(
	  void *
	, const void *
	, atomic::AtomicsHolderAbstract *)
{
	return true; // a nop translator;
}

GuardTransFn 
ExerciseFunctionMaps::lookup_guard_translator(
		  const char * guardname
                , const char * unionhash
                , const char * hash
                , int wtype
		, bool late) const
{
	_atomic_set->get(guardname).set_wtype(wtype);
	return exercise_guard_trans_function;
}

atomic::AtomicMapAbstract * 
ExerciseFunctionMaps::lookup_atomic_map(const char * guardname) const
{
	return _atomic_set->get(guardname).atomic_map();
}

FlatIOConversionFun
ExerciseFunctionMaps::lookup_io_conversion(const char *,const char *) const
{
	return NULL;  // should be safe
}


} // namespace flow
} // namespace oflux
