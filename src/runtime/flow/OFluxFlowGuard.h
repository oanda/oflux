#ifndef OFLUX_FLOW_GUARD
#define OFLUX_FLOW_GUARD


#include "OFlux.h"
#include "OFluxOrderable.h"
#include "atomic/OFluxAtomic.h"
#include <string>

namespace oflux {
namespace flow {

class Flow;

/**
 * @class Guard
 * @brief holder of a guared (used to implement atomic constraints)
 */
class Guard : public MagicNumberable {
public:
	typedef Flow ParentObjType;

        Guard(atomic::AtomicMapAbstract * amap, const char * n, bool is_gc)
                : _amap(amap)
                , _name(n)
		, _is_gc(is_gc)
                {}
        /**
         * @brief acquire the atomic value if possible -- otherwise should wait
         * @param av_returned output argument the Atomic for this key
         * @param key value used to dereference the atomic map
         * @returns key that was permanently allocated on the heap
         */
        inline const void * 
	get(      atomic::Atomic * & av_returned
		, const void * key)
        { return _amap->get(av_returned,key); }

        /**
         * @brief a comparator function for keys (punts to underlying atomic map)
         * @return -1 if <, 0 if ==, +1 if >
         */
        inline int 
	compare_keys(const void *k1, const void * k2) const 
        { return _amap->compare(k1,k2); }
        /**
         * @brief allocate a new key and return a void pointer to it
         * @return the key pointer
         */
        inline void * 
	new_key() const { return _amap->new_key(); }
        /**
         * @brief delete the allocated key
         * @param k the key pointer to reclaim
         */
        inline void 
	delete_key(void * k) const { _amap->delete_key(k); }
        /**
         * @brief return the name of the guard
         * @return name of the guard
         */
        inline const std::string & 
	getName() { return _name; }
        /**
         * @brief release all events from this guard's queues
         */
        void drain();
	void log_snapshot() { if(_amap) _amap->log_snapshot(_name.c_str()); }
	inline bool garbage_collect(const void * key, atomic::Atomic * a)
 	{
 		bool res = false;
 		if(_is_gc) {
 			res = _amap->garbage_collect(key,a);
 		}
 		return res;
 	}
	bool isGC() const { return _is_gc; }
private:
        atomic::AtomicMapAbstract * _amap;
        std::string _name;
	bool _is_gc;
};

/**
 * @class GuardLocalKey
 * @brief hold a local guard key and dispose of it when done
 */
class GuardLocalKey {
public:
	GuardLocalKey(Guard *g)
		: _local_key(g->new_key())
		, _guard(g)
	{}
	~GuardLocalKey()
	{ _guard->delete_key(_local_key); }
	void * get() { return _local_key; }
private:
	void * _local_key;
	Guard * _guard;
};

class Node;

/**
 * @class GuardReference
 * @brief This is the guard instance part of the flow
 */
class GuardReference {
public:
	typedef Node ParentObjType;
	
        GuardReference(Guard * fg, int wtype, bool late)
                : _guardfn(NULL)
                , _flow_guard(fg)
                , _wtype(wtype)
		, _late(late)
                , _lexical_index(-1)
	{}
        ~GuardReference()
        {
        }
        /**
         * @brief set the guard translator function for this guard instance
         */
        inline void 
	setGuardFn(GuardTransFn guardfn) { _guardfn = guardfn; }
        /**
         * @brief used to dereference the atomic map to get the atomic 
         *        and persistent key
         * @param a_out output atomic pointer
         * @param node_in the node input structure (used to generate a key)
         */
        inline const void * 
	get(      atomic::Atomic * & a_out
		, const void * node_in
		, atomic::AtomicsHolderAbstract * ah)
        { 
                bool ok = true;
		GuardLocalKey local_key(_flow_guard);
		try {
			if(_guardfn) {
				ok = (*_guardfn)(local_key.get(), node_in, ah);
			}
		} catch (AHAException & ahae) {
			ok = false;
			log_aha_warning(ahae.func(),ahae.index());
		}
		if(!ok) {
			a_out = NULL;
			return NULL;
		}
                return _flow_guard->get(a_out,local_key.get());
        }
	static void log_aha_warning(const char *, int);
        /**
         * @brief compare keys (pointers to void)
         */
        inline int 
	compare_keys(const void * k1, const void * k2)
        {
                return _flow_guard->compare_keys(k1,k2);
        }
        /**
         * @brief return the magic number of the guard
         */
        inline int 
	magic_number() const { return _flow_guard->magic_number(); }
        /** 
         * @brief return the name of the guard
         */
        inline const std::string & getName() { return _flow_guard->getName(); }

        inline int wtype() const { return _wtype; }
        inline void setLexicalIndex(int i) { _lexical_index = i; }
        inline int getLexicalIndex() const { return _lexical_index; }
	inline bool late() const { return _late || _flow_guard->isGC(); }
 	inline bool garbage_collect(const void * key, atomic::Atomic * a)
 	{
 		return _flow_guard->garbage_collect(key,a);
 	}
private:
        GuardTransFn _guardfn;
        Guard *      _flow_guard;
        int          _wtype;
	bool         _late;
        int          _lexical_index;
};

} // namespace flow
} // namespace oflux

#endif // OFLUX_FLOW_GUARD
