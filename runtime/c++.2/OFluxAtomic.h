#ifndef _OFLUX_ATOMIC
#define _OFLUX_ATOMIC

#include <deque>
#include <map>
#include <boost/shared_ptr.hpp>

namespace oflux {

/**
 * @file OFluxAtomic.h
 * @author Mark Pichora
 * @brief These classes are used to implement the runtime aspect of atomic guards
 */

class EventBase;

/**
 * @class Atomic
 * @brief An atomic represents the instance of a guard, and it holds the needed state
 * Atomics also hold a single private data pointer to let this object 
 * hold the persistent data associated with this guard and the specific key
 * used to obtain it.
 */
class Atomic {
public:
	Atomic(void * data)
		: _data_ptr(data)
		, _held(false)
		{}
	/**
         * @brief return a pointer to the local data pointer (can mod)
         * @return the location where the pointer is.
         */
	inline void ** data() { return &_data_ptr; }
	/**
  	 * @brief tells us if the atomic is held by an event
  	 * @return true if the atomic is held by some event
  	 */
	inline bool held() const { return _held; }
	/**
	 * @brief attempt to aquire the atomic (will succeed if it is not already held)
	 * @return true if we have succeeded in obtaining the atomic
	 */
	inline bool acquire()
		{ 
			bool res = !_held;
			_held = true;
			return res;
		}
	/**
	 * @brief release the atomic (should be done by the holder)
	 * @return a (shared) pointer to the "next" event to get the atomic
	 * If no event was in the waiting list, then the returned value is NULL
	 */
	inline boost::shared_ptr<EventBase> release() 
		{ 
			_held = false; 
			if(_waiters.size()) {
				boost::shared_ptr<EventBase> ev = _waiters.front();
				_waiters.pop_front();
				return ev;
			} else {
				return _null_static;
			}
		}
	/**
         * @brief add an envent the waiting list of this atomic
         * It is assumed that the atomic is held by another event
         */
	inline void wait(boost::shared_ptr<EventBase> ev)
		{ _waiters.push_back(ev); }
	/**
	 * @brief obtain the size of the waiting list
	 * @return the number of events in the waiting list
	 */
	inline int waiter_count() { return _waiters.size(); }
private:
	void *                                    _data_ptr;
	bool                                      _held;
	std::deque<boost::shared_ptr<EventBase> > _waiters;
	static boost::shared_ptr<EventBase>       _null_static;
};

/**
 * @class AtomicMapAbstract
 * @brief Abstract class for an AtomicMap - which holds the key/Atomic relation
 * These maps are used in the runtime to obtain the atomics from the guard
 * arguments (collectively called the keys)
 */
class AtomicMapAbstract {
public:
	virtual ~AtomicMapAbstract() {}
	/**
	 * @brief grab the atomic with a given key
	 * @return a pointer to a persistent key (value-identical to the 
	 * provided key)
	 */
	virtual const void * get(Atomic * & a_out,const void * key) = 0;
	/**
	 * @brief compare two keys (standard return)
	 * @param v_k1 left hand key
	 * @param v_k2 right hand key
	 * @return 0 if equal, 1 if greater, -1 if less than
	 */
	virtual int compare (const void * v_k1, const void * v_k2) const = 0;
	/**
	 * @brief allocate a new key
	 */
	virtual void * new_key() = 0;
	/**
	 * @brief deallocate a key
	 */
	virtual void delete_key(void *) = 0;
};

/**
 * @class AtomicMapTrivial
 * @brief implementation of the AtomicMap interface for keys that are empty
 * When there is only one possible key, there is no need to pass arguments
 */
class AtomicMapTrivial : public AtomicMapAbstract {
public:
	AtomicMapTrivial() 
		: _atomic(NULL)
		{}
	virtual ~AtomicMapTrivial() {}
	virtual const void * get(Atomic * & a_out,const void * /*key*/)
	{
		a_out = &_atomic;
		return reinterpret_cast<void *> (&_something);
	}
	virtual int compare(const void*, const void*) const { return 0; } // eq
	virtual void * new_key() { return NULL; }
	virtual void delete_key(void *) {}
private:
	int    _something;
	Atomic _atomic;
};

/**
 * @class AtomicMapStdMap
 * @brief this is the standard atom map
 * Keys are compared given the operator< in the key class K
 */
template<typename K>
class AtomicMapStdMap : public AtomicMapAbstract {
public:
	AtomicMapStdMap() {}
	class Cmp {
	public:
		bool operator()( const K* s1, const K* s2 ) const 
		{
			  return *s1 < *s2;
		}
	};
	virtual ~AtomicMapStdMap() 
	{
		typename std::map<const K*, Atomic*,Cmp>::const_iterator mitr = _map.begin();
		while(mitr != _map.end()) {
			delete (*mitr).first;
			delete (*mitr).second;
		}
	}
	virtual const void *get(Atomic * &atomic, const void * key)
	{
		const K * k = reinterpret_cast<const K*>(key);
		typename std::map<const K*, Atomic*,Cmp>::const_iterator mitr = _map.find(k);
		if(mitr == _map.end()) {
			typename std::pair<K*,Atomic*> vp(new K(*k),new Atomic(NULL));
			typename std::pair< typename std::map<const K*,Atomic*,Cmp>::iterator,bool> ir =
				_map.insert(vp);
			mitr = ir.first;
		}
		atomic = (*mitr).second;
		return reinterpret_cast<const void *>((*mitr).first);
	}
	virtual int compare(const void * v_k1, const void * v_k2) const
	{
		const K * k1 = reinterpret_cast<const K *>(v_k1);
		const K * k2 = reinterpret_cast<const K *>(v_k2);
		return (*k1 < *k2 ? -1 : (*k2 < *k1 ? 1 : 0));
	}
	virtual void * new_key() { return new K(); }
	virtual void delete_key(void * k) { delete (reinterpret_cast<const K *>(k)); }
private:
	std::map<const K*, Atomic*,Cmp> _map;
};

};

#endif // _OFLUX_ATOMIC
