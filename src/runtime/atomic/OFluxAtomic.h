#ifndef _OFLUX_ATOMIC
#define _OFLUX_ATOMIC

/**
 * @file OFluxAtomic.h
 * @author Mark Pichora
 * @brief These classes are used to implement the runtime aspect of 
 *        atomic guards
 */


#include <deque>
#include <map>

#ifdef Darwin
#include <boost/tr1/tr1/unordered_map>
#else
#include <tr1/unordered_map>
#endif
#include <vector>
#include "OFlux.h"
#include "event/OFluxEventBase.h"
#include "OFluxLogging.h"

namespace oflux {

class EventBase;

namespace atomic {


/**
 * @class Atomic
 * @brief An atomic represents the instance of a guard, 
 *        and it holds the needed state
 *
 * Atomics also hold a single private data pointer to let this object
 * hold the persistent data associated with this guard and the specific key
 * used to obtain it.
 */
class Atomic {
public:
	virtual ~Atomic() {}
	/**
	* @brief return a pointer to the local data pointer (can mod)
	* @return the location where the pointer is.
	*/
	virtual void ** data() = 0;
	/**
	* @brief tells us if the atomic is held by an event
	* @return true if the atomic is held by some event
	*/
	virtual int held() const = 0;
	/**
	* @brief release the atomic (should be done by the holder)
	* @param rel_ev container for waiters released to run (or acquire more)
	* @param by_ev is the event that is causing the release
	* If no event was in the waiting list, then the returned value is NULL
	*/
	virtual void release(std::vector<EventBasePtr> & rel_ev
		, EventBasePtr & by_ev) = 0;
	/**
	* @brief Either acquire it or add an event the waiting list of 
	* this atomic.
	* Acquisition fails when the atomic is held by another event.
	* @param ev event which would wait on failure
	* @param wtype mode of the acquire
	* @return true when acquired and false when wait-ed
	*/
	virtual bool acquire_or_wait(EventBasePtr & ev,int wtype) = 0;
	/**
	* @brief obtain the size of the waiting list
	* @return the number of events in the waiting list
	*/
	virtual size_t waiter_count() = 0;

	virtual bool has_no_waiters() { return waiter_count() == 0; }

	/**
	* @brief let go of this object -- a shorter release when you know
	*    there are no events using it
	*/
	virtual void relinquish(bool) = 0;

	virtual int wtype() const = 0;

	virtual const char * atomic_class() const = 0;

	virtual void log_snapshot_waiters() const {}

	virtual bool is_pool_like() const { return false; }

	static EventBasePtr _null_static;
};

class AtomicFree : public Atomic {
public:
	enum { Free = 6 };
	AtomicFree(void *datap) : _data_ptr(datap) {}
	virtual ~AtomicFree() {}
	virtual void ** data() { return &_data_ptr; }
	virtual int held() const { return 1; } // always available
	virtual void release(std::vector<EventBasePtr > & 
		, EventBasePtr &) 
	{}
	virtual bool acquire_or_wait(EventBasePtr &,int ) 
	{ return 1; }
	virtual size_t waiter_count() { return 0; }
	virtual void relinquish(bool) {}
	virtual int wtype() const { return 0; }
	virtual const char * atomic_class() const { return "Free     "; }
private:
	void * _data_ptr;
};

class AtomicCommon : public Atomic {
public:
	enum { None = 0 };
	AtomicCommon(void * data)
		: _data_ptr(data)
		, _held(0)
	{}
	virtual ~AtomicCommon() {}
	virtual void ** data() { return &_data_ptr; }
	virtual int held() const { return _held; }
	virtual size_t waiter_count() { return _waiters.size(); }
	virtual void relinquish(bool) {}

	struct AtomicQueueEntry {
		AtomicQueueEntry(EventBasePtr & ev,
		int wt)
		: event(ev)
		, wtype(wt)
		{}
		AtomicQueueEntry(const AtomicQueueEntry & o)
			: event(o.event)
			, wtype(o.wtype)
		{}
		EventBasePtr event;
		int wtype;
	};

	virtual void log_snapshot_waiters() const;
protected:
	void *                              _data_ptr;
	int                                 _held;
	std::deque<AtomicQueueEntry>        _waiters;
};

class AtomicMapWalker {
public:
	virtual ~AtomicMapWalker() {}
	virtual bool next(const void * & key,Atomic * &atom) = 0;
};

class AtomicExclusive : public AtomicCommon { // implements AtomicScaffold
public:
	enum { Exclusive = 3 };
	AtomicExclusive(void * data)
		: AtomicCommon(data)
	{}
	virtual ~AtomicExclusive() {}
	virtual void release(std::vector<EventBasePtr > & rel_ev
		, EventBasePtr &)
	{
		_held = 0;
		if(_waiters.size()) {
			rel_ev.push_back(_waiters.front().event);
			_waiters.pop_front();
			_held = 1; // released ev has acquired
		}
	}
	virtual bool acquire_or_wait(EventBasePtr & ev, int)
	{
		bool res = !_held;
		_held = 1;
		if(!res) {
			AtomicQueueEntry aqe(ev,Exclusive);
			_waiters.push_back(aqe);
		}
		return res;
	}
	virtual int wtype() const { return 3; }
	virtual const char * atomic_class() const { return "Exclusive"; }
};

class AtomicReadWrite : public AtomicCommon { // implements AtomicScaffold
public:
	enum { Read = 1, Write = 2, Upgradeable = 4 } WType;
	AtomicReadWrite(void * data)
		: AtomicCommon(data)
		, _mode(AtomicCommon::None)
		{}
	virtual ~AtomicReadWrite() {}
	virtual void release(std::vector<EventBasePtr > & rel_ev
		, EventBasePtr &)
	{
                oflux_log_trace2(" ARW::release() called held:%d mode:%d waiters:%d\n"
                        , _held
                        , _mode
                        , _waiters.size());
		_held = std::max(_held-1,0);
		if(_held == 0) {
			_mode = AtomicCommon::None;
			if(_waiters.size()) {
				AtomicQueueEntry & aqe = _waiters.front();
				_mode = aqe.wtype;
				if(_mode == Upgradeable) {
					_mode = (*data() ? Read : Write);
				}
				rel_ev.push_back(aqe.event);
				_mode = aqe.wtype;
				++_held;
				_waiters.pop_front();
				if(_mode == Read) {
					while(_waiters.size() && (_waiters.front().wtype == Read 
							|| (*data() && _waiters.front().wtype == Upgradeable))) {
						rel_ev.push_back(_waiters.front().event);
						++_held;
						_waiters.pop_front();
					}
				}
			}
		}
                oflux_log_trace2(" ARW::release() finished held:%d mode:%d releases:%d\n"
                        , _held
                        , _mode
                        , rel_ev.size());
	}
	virtual bool acquire_or_wait(EventBasePtr & ev, int wtype)
	{
                int res = 0;
                oflux_log_trace2(" ARW::acquire() called wtype:%d held:%d mode:%d waiters:%d\n"
                        , wtype
                        , _held
                        , _mode
                        , _waiters.size());
		if(_held == 0) {
			_held = 1;
			_mode = ( wtype == Upgradeable 
				? (*data() ? Read : Write)
				: wtype);
			_mode = wtype;
			res = 1;
		} else if(_mode == Read && _waiters.size() == 0
				&& ((wtype == Upgradeable && *data())
					|| (wtype == Read))) {
			_held++;
			res = 1;
		}
                oflux_log_trace2(" ARW::acquire() finished held:%d mode:%d res:%d\n"
                        , _held
                        , _mode
                        , res);
                oflux_log_trace2(" ARW::wait() called wtype:%d held:%d mode:%d waiters:%d\n"
                        , wtype
                        , _held
                        , _mode
                        , _waiters.size());
		if(!res) {
			AtomicQueueEntry aqe(ev,wtype);
			_waiters.push_back(aqe);
		}
		return res;
	}
	virtual int wtype() const { return _mode; }
	virtual const char * atomic_class() const { return "ReadWrite"; }
private:
	int _mode; // should only be either {Read,Write}
};

/****
class AtomicSequence { // implements AtomicScaffold
public:
    AtomicSequence(int seq_number)
        : _seq_number(seq_number)
        , _to_sn(&_seq_number)
        {}
    inline void ** data() { return &_to_sn; }
    inline int held() const { return 0; }
    inline int waiter_count() { return 0; }
    inline int acquire(int) { return 1; }
    inline void release(std::vector<boost::shared_ptr<EventBase> > & rel_ev))
        { }
    inline void wait(boost::shared_ptr<EventBase> & ev, int wtype)
        { assert(0); }
private:
    int _seq_number;
    int * _to_sn;
};
****/

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
	virtual void * new_key() const = 0;
	/**
	 * @brief deallocate a key
	 */
	virtual void delete_key(void *) const = 0;
	/**
	 * @brief get a walker for this map
	 */
	virtual AtomicMapWalker * walker() = 0;

	virtual void log_snapshot(const char * guardname);

	virtual bool garbage_collect(const void *, Atomic *) { return false; } 
		// no gc by default
};

class AtomicPooled;

class AtomicPoolWalker : public AtomicMapWalker {
public:
	AtomicPoolWalker(AtomicPooled *n)
		: _n(n)
	{}
	virtual ~AtomicPoolWalker() {}
	virtual bool next(const void * & key,Atomic * &atom);
private:
	AtomicPooled * _n;
};

class AtomicPool : public AtomicMapAbstract {
public:
	friend class AtomicPooled;
	AtomicPool ()
		: _ap_list(NULL)
                , _ap_last(NULL)
	{}
	virtual ~AtomicPool();
	void * get_data();
	void put_data(void *);
	EventBasePtr get_waiter();
	void put_waiter(EventBasePtr & ev);
	size_t waiter_count() { return _q.size(); }
	virtual int compare (const void * v_k1, const void * v_k2) const { return 0; }
	virtual void * new_key() const { return NULL; }
	virtual void delete_key(void *) const {}
	virtual const void * get(Atomic * & a_out,const void * key);
	void put(AtomicPooled * ap);
	virtual AtomicMapWalker * walker() { return new AtomicPoolWalker(_ap_list); }
protected:
	void log_snapshot_waiters() const;
private:
	std::deque<EventBasePtr > _q;
	std::deque<void *> _dq;
	AtomicPooled * _ap_list;
	AtomicPooled * _ap_last;
};

class AtomicPooled : public Atomic { // implements AtomicScaffold
public:
	enum { Pool = 5 };
	AtomicPooled(AtomicPool & pool,void * data)
		: _data(data)
		, _pool(pool)
		, _next(NULL)
	{}
	virtual bool is_pool_like() const { return true; }
	virtual void ** data() { return &_data; }
	virtual int held() const { return _data != NULL; }
	virtual size_t waiter_count() { return _pool.waiter_count(); }
	virtual void release(std::vector<EventBasePtr > & rel_ev
		, EventBasePtr &);
	virtual bool acquire_or_wait(EventBasePtr & ev, int)
	{ 
		_data = _pool.get_data();
		bool res = (_data != NULL);
		oflux_log_trace2("apd::a_o_w() [%d] %p no wc %p data %d res\n", oflux_self(), this, _data, res);
		if(!res) {
			_pool.put_waiter(ev); 
		}
		return res;
	}
	virtual void * new_key() const { return NULL; }
	virtual void delete_key(void *) const {}
	virtual void relinquish(bool should)
	{
		if(should) {
			oflux_log_trace2("apd::relinq() [%d] %p no wc %p data\n", oflux_self(), this, _data);
			if(_data) {
				_pool.put_data(_data);
				_data = NULL;
			}
			_pool.put(this);
		}
	}
	virtual int wtype() const { return 0; }
	static const char * atomic_class_str;
	virtual const char * atomic_class() const { return atomic_class_str; }
	virtual void log_snapshot_waiters() const { return _pool.log_snapshot_waiters(); }
public:
	AtomicPooled * next() { return _next; }
	void next(AtomicPooled * ap) { _next = ap; }
private:
	void *         _data;
	AtomicPool &   _pool;
	AtomicPooled * _next;
};

class TrivialWalker : public AtomicMapWalker {
public:
	TrivialWalker(Atomic &a)
		: _atom(a)
		, _more(true)
		{}
	virtual ~TrivialWalker() {}
	virtual bool next(const void * & key,Atomic * &atom);
private:
	Atomic & _atom;
	bool _more;
};

/**
 * @class AtomicMapTrivial
 * @brief implementation of the AtomicMap interface for keys that are empty
 * When there is only one possible key, there is no need to pass arguments
 */
template<typename A=AtomicExclusive>
class AtomicMapTrivial : public AtomicMapAbstract {
public:
	AtomicMapTrivial()
		: _something(0)
		, _atomic(NULL)
		{}
	virtual ~AtomicMapTrivial() {}
	virtual const void * get(Atomic * & a_out,const void * /*key*/)
	{
		a_out = &_atomic;
		return reinterpret_cast<void *> (&_something);
	}
	virtual int compare(const void*, const void*) const { return 0; } // eq
	virtual void * new_key() const { return NULL; }
	virtual void delete_key(void *) const {}
	virtual AtomicMapWalker * walker() { return new TrivialWalker(_atomic); }
private:
	int _something;
	A   _atomic;
};

template<typename K>
class AtomicMapStdCmp {
public:
	bool operator()( const K* s1, const K* s2 ) const
	{
		return *s1 < *s2;
	}
};

template<typename K>
class AtomicMapUnorderedCmp {
public:
	bool operator()( const K* s1, const K* s2 ) const
	{
		return *s1 == *s2;
	}
};

template<typename K>
class BaseMapPolicy {
public:
        typedef K keytype;
        typedef std::pair<const K *, Atomic *> pairtype;
};

template<typename K>
class StdMapPolicy : public BaseMapPolicy<K> {
public:
        typedef typename std::map<const K*, Atomic*,AtomicMapStdCmp<K> > maptype;
        typedef typename maptype::iterator iterator;
        typedef typename maptype::const_iterator const_iterator;
        typedef typename std::pair<iterator, bool> insertresulttype;
};

template<typename K>
class HashMapPolicy : public BaseMapPolicy<K> {
public:
        typedef typename std::tr1::unordered_map<const K*, Atomic*, hash_ptr<K>, AtomicMapUnorderedCmp<K> > maptype;
        typedef typename maptype::iterator iterator;
        typedef typename maptype::const_iterator const_iterator;
        typedef typename std::pair<iterator,bool> insertresulttype;
};

template<typename MapPolicy>
class AtomicMapStdWalker : public AtomicMapWalker {
public:
	AtomicMapStdWalker( typename MapPolicy::maptype & map)
		: _at(map.begin())
		, _end(map.end())
		{}
	virtual ~AtomicMapStdWalker() {}
	virtual bool next(const void * & key,Atomic * &atom)
	{
		bool res = _at != _end;
		if(res) {
			key = (*_at).first;
			atom = (*_at).second;
			_at++;
		}
		return res;
	}

private:
	typename MapPolicy::iterator _at;
	typename MapPolicy::iterator _end;
};

template<typename K>
inline int
k_compare(const void * v_k1, const void * v_k2) 
{
	if (v_k1 && v_k2) {
		const K * k1 = 
			reinterpret_cast<const K *>(v_k1);
		const K * k2 = 
			reinterpret_cast<const K *>(v_k2);
		return (*k1 < *k2 ? -1 : (*k2 < *k1 ? 1 : 0));
	} else if (!v_k1 && v_k2) {
		return -1;
	} else if (v_k1 && !v_k2) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * @class AtomicMapStdMap
 * @brief this is the standard atom map
 * Keys are compared given the operator< in the key class K
 */
template<typename MapPolicy,typename A=AtomicExclusive>
class AtomicMapStdMap : public AtomicMapAbstract {
public:
	AtomicMapStdMap() {}
	virtual ~AtomicMapStdMap()
	{
		typename MapPolicy::const_iterator mitr = _map.begin();
		while(mitr != _map.end()) {
			delete (*mitr).first;
			delete (*mitr).second;
			mitr++;
		}
	}
	virtual const void *get(Atomic * &atomic, const void * key)
	{
                const typename MapPolicy::keytype * k = 
                        reinterpret_cast<const typename MapPolicy::keytype *>(key);
                typename MapPolicy::const_iterator mitr = _map.find(k);
                if(mitr == _map.end()) {
                        typename MapPolicy::pairtype vp(
                                  new typename MapPolicy::keytype(*k)
                                , new A(NULL));
                        typename MapPolicy::insertresulttype ir =
                                _map.insert(vp);
			mitr = ir.first;
		}
		atomic = (*mitr).second;
		return reinterpret_cast<const void *>((*mitr).first);
	}
	virtual int compare(const void * v_k1, const void * v_k2) const
	{ return k_compare<typename MapPolicy::keytype>(v_k1,v_k2); }
	virtual void * new_key() const 
	{ return new typename MapPolicy::keytype(); }
	virtual void delete_key(void * k) const
        { delete (reinterpret_cast<const typename MapPolicy::keytype *>(k)); }
	virtual AtomicMapWalker * walker() 
        { return new AtomicMapStdWalker<MapPolicy>(_map); }
	virtual bool garbage_collect(const void * key, Atomic * a)
	{
                const typename MapPolicy::keytype * k = 
                        reinterpret_cast<const typename MapPolicy::keytype *>(key);
                typename MapPolicy::iterator mitr = _map.find(k);
		if(_map.end() != mitr) {
			assert(((*mitr).second == a) 
				&& "garbage_collect detected that atom object "
				   "does not match what is in the map");
			assert(((*mitr).first == k) 
				&& "garbage_collect detected that key object "
				   "does not match what is in the map");
			_map.erase(mitr);
			delete a;
			delete_key(const_cast<void*>(key));
			return true;
		}
		return false;
	}
private:
	typename MapPolicy::maptype _map;
};

} // namespace atomic
} // namespace oflux

#endif // _OFLUX_ATOMIC
