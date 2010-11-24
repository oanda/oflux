#include "flow/OFluxFlowExerciseFunctions.h"
#include "flow/OFluxFlowLibrary.h"
#include "event/OFluxEvent.h"
#include "atomic/OFluxAtomicHolder.h"
#include "atomic/OFluxAtomicInit.h"
#include "lockfree/atomic/OFluxLFAtomic.h"
#include "lockfree/atomic/OFluxLFAtomicPooled.h"
#include "lockfree/atomic/OFluxLFAtomicReadWrite.h"
#include "OFluxLogging.h"
#include <cassert>
#include <string>
#include <strings.h>
#include <string.h>
#include <unistd.h>

namespace oflux {
namespace flow {

namespace exercise {

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
	populator.insert(NULL,new int(0));
}

class LFAtomic : public AtomicAbstract {
public:
	typedef oflux::atomic::AtomicMapTrivial<oflux::lockfree::atomic::AtomicExclusive> AtomicExclusive;
	typedef oflux::atomic::AtomicMapTrivial<oflux::lockfree::atomic::AtomicReadWrite> AtomicReadWrite;
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
private:
	ManyThings<oflux::atomic::AtomicMapAbstract, max_size> _many;
	AtomicExclusive _exclusive;
	AtomicReadWrite _readwrite;
	AtomicFree _free;
	AtomicPool _pool;
};

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
	typedef oflux::atomic::AtomicMapTrivial<oflux::atomic::AtomicExclusive> AtomicExclusive;
	typedef oflux::atomic::AtomicMapTrivial<oflux::atomic::AtomicReadWrite> AtomicReadWrite;
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
private:
	ManyThings<oflux::atomic::AtomicMapAbstract, max_size> _many;
	AtomicExclusive _exclusive;
	AtomicReadWrite _readwrite;
	AtomicFree _free;
	AtomicPool _pool;
};

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
private:
	int style;
	std::map<std::string,AtomicAbstract *> _map;
};

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

		std::string node_name;
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
	number = std::min(MAX_ATOMICS_PER_NODE,ah->number());
	for(int i = 0; i < number; ++i) {
		oflux::atomic::Atomic * atomic = ah->get(i,oflux::atomic::AtomicsHolder::HA_get_lexical)->atomic();
		atoms[i].data = atomic->data();
		atoms[i].wtype = atomic->wtype();
	}
}

int
exercise_node_function(
	  const ExerciseEventDetail::In_ * in
	, ExerciseEventDetail::Out_ * out
	, ExerciseEventDetail::Atoms_ * atoms)
{
	oflux_log_debug("exercise_node_function %s %d\n"
		, atoms->node_name.c_str()
		, in->value);
	atoms->report();
	out->value = in->value;
	return 0;
}

int
exercise_error_node_function(
	  const ExerciseEventDetail::In_ * in
	, ExerciseEventDetail::Out_ *
	, ExerciseEventDetail::Atoms_ * atoms
	, int)
{
	oflux_log_debug("exercise_error_node_function %s %d\n"
		, atoms->node_name.c_str()
		, in->value);
	atoms->report();
	return 0;
}

int
exercise_source_node_function(
	  const ExerciseEventDetail::In_ *
	, ExerciseEventDetail::Out_ * out
	, ExerciseEventDetail::Atoms_ * atoms)
{
	static unsigned seed = 334;
	out->value = rand_r(&seed);
	oflux_log_debug("exercise_source_node_function %s %d\n"
		, atoms->node_name.c_str()
		, out->value);
	atoms->report();
	::usleep(1); // if sources do not stop a little - bad stuff happens
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


EventBasePtr
create_special( 
	  EventBasePtr pred_node_ptr
        , const void * im_io_convert
        , flow::Node *fn)
{
	if(fn->getIsSource()) {
		// special case for sources
		EventBaseTyped<ExerciseSourceEventDetail> * ebt = 
			new Event<ExerciseSourceEventDetail>(
			  pred_node_ptr
			, reinterpret_cast<const IOConversionBase<ExerciseSourceEventDetail::In_> *>(im_io_convert)
			, fn);
		ebt->atomics_argument()->node_name = fn->getName();
		return EventBasePtr(ebt);
	} else if(fn->getIsErrorHandler()) {
		EventBaseTyped<ExerciseErrorEventDetail> * ebt = 
			new ErrorEvent<ExerciseErrorEventDetail>(
			  pred_node_ptr
			, reinterpret_cast<const IOConversionBase<ExerciseErrorEventDetail::In_> *>(im_io_convert)
			, fn);
		ebt->atomics_argument()->node_name = fn->getName();
		return EventBasePtr(ebt);
	} else {
		EventBaseTyped<ExerciseEventDetail> * ebt = 
			new Event<ExerciseEventDetail>(
			  pred_node_ptr
			, reinterpret_cast<const IOConversionBase<ExerciseEventDetail::In_> *>(im_io_convert)
			, fn);
		ebt->atomics_argument()->node_name = fn->getName();
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
	oflux_log_trace("  exercise_condition_function %d\n", out->value);
	bool res = (out->value) % 2;
	return res;
}

ConditionFn 
ExerciseFunctionMaps::lookup_conditional(const char * n, int argno, int unionnumber) const
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
                , int union_number
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
ExerciseFunctionMaps::lookup_io_conversion(int,int) const
{
	return NULL;  // should be safe
}


} // namespace flow
} // namespace oflux
