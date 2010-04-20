#ifndef _OFLUX_ATOMIC_HOLDER
#define _OFLUX_ATOMIC_HOLDER


#include <vector>
#include <cassert>
#include "atomic/OFluxAtomic.h"
#include "flow/OFluxFlowGuard.h"
#include "OFluxLibDTrace.h"



namespace oflux {
namespace atomic {

/**
 * @file OFluxAtomicHolder.h
 * @author Mark Pichora
 * A class for acquiring atomic guards at runtime in the right order 
 * (partly based on value)
 */


/**
 * @class HeldAtomic
 * @brief The information that an event should have on hand for a held atomic
 */
class HeldAtomic {
public:

	HeldAtomic()
		: _atom(NULL)
		, _flow_guard_ref(NULL)
		, _key(NULL)
		, _haveit(false)
	{}
        ~HeldAtomic()
	{ relinquish(); }
	/**
	 * @brief init is a pseudo constructor used to tell us which guard
	 * @param fg the FlowGuardReference indicates the guard instance
	 */
	inline void init(flow::GuardReference * fgr)
	{ _flow_guard_ref = fgr; }
	/**
         * @brief the guard type indicates which guard is being used (no key values)
         * @return the integer magic number for the guard
         */
	inline int guard_type() const 
	{ return _flow_guard_ref->magic_number(); }
	//void set_atomic(Atomic * atom, bool haveit)
		//{ _atom = atom; _haveit = haveit; }
	//HeldAtomic & operator=(const HeldAtomic & ha);
	inline flow::GuardReference * flow_guard_ref() 
	{ return _flow_guard_ref; }
	/**
	 * @brief compare is a classical compare function for heldatomics
	 * @remark key values are taken into account in this ordering
	 * @param ha the RHS of the comparison
	 * @return -1 if <, +1 if > and 0 if ==
	 */
	inline int compare(const HeldAtomic & ha) const
	{ 
		int gt1 = guard_type();
		int gt2 = ha.guard_type();
		int wt1 = wtype();
		int wt2 = ha.wtype();
		bool lt1 = _flow_guard_ref->late();
		bool lt2 = ha._flow_guard_ref->late();
#define compare_lates(a,b) \
(a == b ? 0 : ( (!a) && b /*a < b*/ ? -1 : 1 ))
		return (gt1 == gt2 && wt1 == wt2
				? (!lt1 && !lt2
					? _flow_guard_ref->compare_keys(_key,ha._key)
					: compare_lates(lt1,lt2))
				: (gt1 == gt2 
					? (wt1 < wt2 ? -1 : 1)
					: (gt1 < gt2 ? -1 : 1))); 
	}
	inline int compare_keys(const void * k)
	{ return _flow_guard_ref->compare_keys(_key,k); }
	/**
	 * @brief determine if the atomic is held (thus the owner has exclusivity)
	 * @return true if you have it
	 */
	inline bool haveit() { return _haveit; }
	/**
	 * @brief take the held atomic from another
	 * @param ha the held atomic object to steal the atomic from
	 */
	inline void takeit(HeldAtomic & ha) 
	{
		assert(ha._atom);
		assert(ha._haveit);
		relinquish();
		_atom = ha._atom;
		ha._atom = NULL;
		_haveit = ha._haveit;
		ha._haveit = false;
	}
	/**
	 * @brief populate the key given the input node argument
	 * @param node_in a void ptr to the input node data structure
	 */
	inline bool build(const void * node_in
			, AtomicsHolderAbstract * ah
			, bool allow_late = false)
	{ 
		assert(_atom == NULL || allow_late);
		if((allow_late || !_flow_guard_ref->late()) && !_atom) {
			_key = _flow_guard_ref->get(_atom,node_in,ah); 
		}
		//if(_atom == NULL) {
			//oflux_log_info("HeldAtomic::build() conditional guard not held %s\n", _flow_guard->getName().c_str());
		//}
		return _atom != NULL;
	}
	/**
	 * @brief attempt to acquire the atomic (will succeed if no other has it)
	 * @return true if the atomic is now held
	 */
	inline bool acquire_or_wait(EventBasePtr & ev,const char * ev_name)
	{
		assert(_key);
		assert(_atom);
		_haveit = _atom->acquire_or_wait(ev,_flow_guard_ref->wtype());
		if(_haveit) {
			_GUARD_ACQUIRE(
				  flow_guard_ref()->getName().c_str()
				, ev_name
				, 0); 
		} else {
			_GUARD_WAIT(
				  flow_guard_ref()->getName().c_str()
				, ev_name
				, wtype); 
		}
		return _haveit;
	}
	//inline const void * key() { return _key; }
	/**
	 * @brief the underlying atomic object (not necessarily held)
	 */
	inline Atomic * atomic() { return _atom; }
	inline void atomic(Atomic * a) { _atom = a; }
	inline void relinquish()
	{
		if(_atom != NULL) {
			_atom->relinquish();
		}
		_atom = NULL;
	}
	inline int wtype() const { return _flow_guard_ref->wtype(); }
        inline bool skipit() const { return _atom == NULL; }
private:
	Atomic *               _atom;
	flow::GuardReference * _flow_guard_ref;
	const void *           _key;
	bool                   _haveit;
};

// want this to be static and easy
//
#define MAX_ATOMICS_PER_NODE 20

/**
 * @class AtomicsHolder
 * @brief This class is a container for a bunch of HeldAtomics
 */
class AtomicsHolder : public AtomicsHolderAbstract {
public:
	static AtomicsHolder empty_ah;
	
	AtomicsHolder(bool completelysorted = false)
		: _number(0)
		, _is_sorted_and_keyed(false)
                , _is_completely_sorted(completelysorted)
                , _working_on(0)
		{}
	void add(flow::GuardReference * fg);
	/**
	 * @brief given a bunch of held atomics and a node input try to acquire (in order) what you need.
	 * @remark returns true on sucess
	 * @param ev that gets 'waited' onto a queue if waiting is necessary
	 * @param given_atomics (available to be captured
	 * @return true if we succeed in acquiring all that is needed
	 */
	int acquire_all_or_wait(
		  EventBasePtr & ev
		, AtomicsHolder & given_atomics = empty_ah);
	/**
	 * @brief release all the underlying atomics
	 * @param released_events an output vector of events that have been waiting on the freed stuff
	 */
	void release(
		  std::vector<EventBasePtr> & released_events
		, EventBasePtr & by_ev);
	/**
	 * @return the number of atomics held (only some of them are acquired)
	 */
	inline int number() { return _number; }
	/**
	 * @brief get the ith held atomic (can do this in sorted order)
	 * @return ith held atomic
	 */
        enum HA_get { HA_get_unsorted, HA_get_sorted, HA_get_lexical };
	inline HeldAtomic * get(int i, HA_get hg = HA_get_sorted) 
	{ 
		return (_is_sorted_and_keyed && (hg==HA_get_sorted)
				? _sorted[i]
				: (hg == HA_get_lexical)
					? _lexical[i]
					: &(_holders[i])); 
	}
	virtual void * getDataLexical(int i)
	{ 
		HeldAtomic * haptr = _lexical[i];
		Atomic * aptr = (haptr ? haptr->atomic() : NULL);
		return (aptr
			? *(aptr->data())
			: NULL);
	}
		
protected:
	bool get_keys_sort(const void * node_in);
private:
	int          _number;
	HeldAtomic   _holders[MAX_ATOMICS_PER_NODE];
	HeldAtomic * _sorted[MAX_ATOMICS_PER_NODE];
	HeldAtomic * _lexical[MAX_ATOMICS_PER_NODE];
	bool         _is_sorted_and_keyed;
        bool         _is_completely_sorted;
        int          _working_on;
};

} // namespace atomic
} // namespace oflux

#endif // _OFLUX_ATOMIC_HOLDER
