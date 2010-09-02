#ifndef OFLUX_LF_ATOMIC_MAPS
#define OFLUX_LF_ATOMIC_MAPS

#include "lockfree/atomic/OFluxLFAtomic.h"
#include "lockfree/atomic/OFluxLFAtomicReadWrite.h"
#include "lockfree/atomic/OFluxLFHashTable.h"

namespace oflux {
namespace lockfree {
namespace atomic {

// Note: template AtomicMapTrivial is already lock free
//       so no need to implement Trivial maps


template< typename K
	, typename A=oflux::lockfree::atomic::AtomicExclusive >
class AtomicMapUnorderedWalker : public oflux::atomic::AtomicMapWalker {
public:
	//AtomicMapUnorderedWalker(KVEnumeratorRef<K,A> & enumRef)
	AtomicMapUnorderedWalker(KeyValueHashTableEnumerator<K,A> * enumer)
		//: _enumRef(enumRef)
		: _enumer(enumer)
	{}
	virtual ~AtomicMapUnorderedWalker() 
	{ delete _enumer; }
	virtual bool next(const void * & key,oflux::atomic::Atomic * &atom)
	{
		const A * aref = NULL;
		bool res = _enumer->next(
			  reinterpret_cast<const K * &>(key)
			, aref );
		atom = const_cast<A *>(aref);
		return res;
	}
private:
	//KVEnumeratorRef<K,A> _enumRef;
	KeyValueHashTableEnumerator<K,A> * _enumer;
};

template< typename K
	, typename A=oflux::lockfree::atomic::AtomicExclusive >
class AtomicMapUnordered : public oflux::atomic::AtomicMapAbstract {
public:
	typedef HashTable<K,A> Table;
	AtomicMapUnordered() {}
	virtual ~AtomicMapUnordered() {}

	virtual const void *get(
		  oflux::atomic::Atomic * &atomic
		, const void * key)
	{
		const K * k = reinterpret_cast<const K *>(key);
		size_t k_hash = hash<K>()(*k);
		_thread_k_hashes[_tn.index] = k_hash;
		A * res = &_tombstone;
		while(res == &_tombstone) {
			res = const_cast<A *>(_table.get(*k));
		}
		//int path = 0;
		const A * cas_res = NULL;
		if(res == Table::HTC::Does_Not_Exist) {
			// insert it
			res = new A(NULL);
			cas_res = _table.compareAndSwap(
				  *k
				, Table::HTC::Does_Not_Exist
				, res);
			//path = 1;
			if(Table::HTC::Does_Not_Exist != cas_res) { // cas succeeds
				res = const_cast<A *>(_table.get(*k));
				//path=2;
			}
			//printf("%d get k had to cas hash %u casres %d\n"
				//, pthread_self()
				//, k_hash
				//, Table::HTC::Does_Not_Exist != cas_res);
		}
		atomic = res;
		const void * per_key = _table.getPersistentKey(k,k_hash);
		assert(per_key);
		//printf("%d get k %p hash %u a %p\n"
			//, pthread_self()
			//, per_key
			//, k_hash
			//, atomic);
		return per_key;
	}
	virtual int compare(const void * v_k1, const void * v_k2) const
	{ return oflux::atomic::k_compare<K>(v_k1,v_k2); }
	virtual void * new_key() const
	{ return new K(); }
	virtual void delete_key(void *o) const
	{ delete reinterpret_cast<K *>(o); }
	virtual oflux::atomic::AtomicMapWalker * walker()
	{
		KeyValueHashTableEnumerator<K,A> * enumer =
		//KVEnumeratorRef<K,A> enumref = 
			_table.getKeyValues();
		return new AtomicMapUnorderedWalker<K,A>(enumer);
	}
	virtual bool garbage_collect(const void * key, oflux::atomic::Atomic * a)
	{
		bool res = false;
		const K * k = reinterpret_cast<const K *>(key);
		size_t k_hash = hash<K>()(*k);
		//printf("%d gc on k %p\n", pthread_self(), k);
		if(a->held()+a->waiter_count() == 0 && !k_hash_used(k_hash)) {
			if(_table.compareAndSwap(*k,reinterpret_cast<A*>(a),&_tombstone)) {
				if(a->held()+a->waiter_count() == 0 && !k_hash_used(k_hash)) {
					//printf("%d   gc remove on k %p\n", pthread_self(), k);
					_table.remove(*k);
					delete a;
				} else {
					//printf("%d   fail gc on second check %p\n", pthread_self(), k);
					// oops return it to the hash
					_table.compareAndSwap(*k,&_tombstone,reinterpret_cast<A*>(a));
				}
			} else {
				//printf("%d  fail gc on cas k %p\n", pthread_self(), k);
			}
		} else {
			//printf("%d fail gc on init chk %p\n", pthread_self(), k);
		}
		return res;
	}
private:
	inline bool k_hash_used(size_t kh) const
	{
		for(size_t i = 0
				; i < DEFAULT_MEMPOOL_MAX_THREADS
				  && i < ThreadNumber::num_threads
				; ++i) {
			if(_thread_k_hashes[i] == kh) {
				return true;
			}
		}
		return false;
	}
private:
	Table _table;
	static A _tombstone;
	size_t _thread_k_hashes[DEFAULT_MEMPOOL_MAX_THREADS];
};

template< typename K
	, typename A>
A AtomicMapUnordered<K,A>::_tombstone(NULL);

} // namespace atomic
} // namespace lockfree
} // namespace oflux


#endif // OFLUX_LF_ATOMIC_MAPS
