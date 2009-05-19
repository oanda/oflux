#ifndef _OFLUX_ATOMIC_HOLDER
#define _OFLUX_ATOMIC_HOLDER


#include <vector>
#include <cassert>
#include "OFluxAtomic.h"
#include "OFluxFlow.h"



namespace oflux {

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
		, _flow_guard(NULL)
		, _key(NULL)
		, _haveit(false)
		{}
        ~HeldAtomic()
                { relinquish(); }
	/**
	 * @brief init is a pseudo constructor used to tell us which guard
	 * @param fg the FlowGuardReference indicates the guard instance
	 */
	inline void init(flow::GuardReference * fg)
		{ _flow_guard = fg; }
	/**
         * @brief the guard type indicates which guard is being used (no key values)
         * @return the integer magic number for the guard
         */
	inline int guard_type() const { return _flow_guard->magic_number(); }
	//void set_atomic(Atomic * atom, bool haveit)
		//{ _atom = atom; _haveit = haveit; }
	//HeldAtomic & operator=(const HeldAtomic & ha);
	inline flow::GuardReference * flow_guard_ref() 
		{ return _flow_guard; }
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
			return (gt1 == gt2 && wt1 == wt2
					? _flow_guard->compare_keys(_key,ha._key)
					: (gt1 == gt2 
						? (wt1 < wt2 ? -1 : 1)
						: (gt1 < gt2 ? -1 : 1))); 
		}
	inline int compare_keys(const void * k)
		{ return _flow_guard->compare_keys(_key,k); }
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
	inline bool build(const void * node_in)
		{ 
                        assert(_atom == NULL );
                        _key = _flow_guard->get(_atom,node_in); 
                        //if(_atom == NULL) {
                                //oflux_log_info("HeldAtomic::build() conditional guard not held %s\n", _flow_guard->getName().c_str());
                        //}
                        return _atom != NULL;
                }
	/**
	 * @brief attempt to acquire the atomic (will succeed if no other has it)
	 * @return true if the atomic is now held
	 */
	inline bool acquire() 
		{
			assert(_key);
			assert(_atom);
			_haveit = _atom->acquire(_flow_guard->wtype());
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
	inline int wtype() const { return _flow_guard->wtype(); }
        inline bool skipit() const { return _atom == NULL; }
private:
	Atomic *               _atom;
	flow::GuardReference * _flow_guard;
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
class AtomicsHolder {
public:
	AtomicsHolder(bool completelysorted = false)
		: _number(0)
		, _is_sorted_and_keyed(false)
                , _is_completely_sorted(completelysorted)
		{}
	void add(flow::GuardReference * fg);
	/**
	 * @brief given a bunch of held atomics and a node input try to acquire (in order) what you need.
	 * @remark returns -1 on sucess, otherwise the index where the atomic acquisition failed.
	 * @return -1 if we succeed
	 */
	int acquire( AtomicsHolder & given_atomics, const void * node_in, const char * on_behalf_of_event_name );
	/**
	 * @brief release all the underlying atomics
	 * @param released_events an output vector of events that have been waiting on the freed stuff
	 */
	void release( std::vector<boost::shared_ptr<EventBase> > & released_events);
	/**
	 * @return the number of atomics held (only some of them are acquired)
	 */
	inline int number() { return _number; }
	/**
	 * @brief get the ith held atomic (can do this in sorted order)
	 * @return ith held atomic
	 */
	inline HeldAtomic * get(int i, bool sortedifsorted=true) 
		{ return (_is_sorted_and_keyed && sortedifsorted
				? _sorted[i]
				: &(_holders[i])); }
protected:
	void get_keys_sort(const void * node_in);
private:
	int          _number;
	HeldAtomic   _holders[MAX_ATOMICS_PER_NODE];
	HeldAtomic * _sorted[MAX_ATOMICS_PER_NODE];
	bool         _is_sorted_and_keyed;
        bool         _is_completely_sorted;
};

}; // namespace

#endif // _OFLUX_ATOMIC_HOLDER
