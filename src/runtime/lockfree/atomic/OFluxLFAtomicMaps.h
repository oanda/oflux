#ifndef OFLUX_LF_ATOMIC_MAPS
#define OFLUX_LF_ATOMIC_MAPS

#include "lockfree/atomic/OFluxLFAtomic.h"
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
	AtomicMapUnorderedWalker(KVEnumeratorRef<K,A> & enumRef)
		: _enumRef(enumRef)
	{}
	virtual ~AtomicMapUnorderedWalker() {}
	virtual bool next(const void * & key,oflux::atomic::Atomic * &atom)
	{
		const A * aref = NULL;
		bool res = _enumRef.next(
			  reinterpret_cast<const K * &>(key)
			, aref );
		atom = const_cast<A *>(aref);
		return res;
	}
private:
	KVEnumeratorRef<K,A> _enumRef;
};

template< typename K
	, typename A=oflux::lockfree::atomic::AtomicExclusive >
class AtomicMapUnordered : public oflux::atomic::AtomicMapAbstract {
public:
	typedef HashTable<K,A,16 /*for now*/ > Table;
	AtomicMapUnordered() {}
	virtual ~AtomicMapUnordered() {}

	virtual const void *get(
		  oflux::atomic::Atomic * &atomic
		, const void * key)
	{
		const K * k = reinterpret_cast<const K *>(key);
		A * res = const_cast<A *>(_table.get(*k));
		if(res == Table::HTC::Does_Not_Exist) {
			// insert it
			res = new A(NULL);
			const A * cas_res = _table.compareAndSwap(
				  *k
				, Table::HTC::Does_Not_Exist
				, res);
			if(Table::HTC::Does_Not_Exist != cas_res) { // cas succeeds
				res = const_cast<A *>(_table.get(*k));
			}
		}
		atomic = res;
		return _table.getPersistentKey(res);
	}
	virtual int compare(const void * v_k1, const void * v_k2) const
	{ return oflux::atomic::k_compare<K>(v_k1,v_k2); }
	virtual void * new_key() const
	{ return new K(); }
	virtual void delete_key(void *o) const
	{ delete reinterpret_cast<K *>(o); }
	virtual oflux::atomic::AtomicMapWalker * walker()
	{
		KVEnumeratorRef<K,A> enumref = _table.getKeyValues();
		return new AtomicMapUnorderedWalker<K,A>(enumref);
	}
private:
	Table _table;
};

} // namespace atomic
} // namespace lockfree
} // namespace oflux


#endif // OFLUX_LF_ATOMIC_MAPS
