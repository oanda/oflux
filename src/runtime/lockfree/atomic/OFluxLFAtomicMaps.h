#ifndef OFLUX_LF_ATOMIC_MAPS
#define OFLUX_LF_ATOMIC_MAPS

#include "lockfree/atomic/OFluxAtomic.h"
#include "lockfree/atomic/OFluxLFHashTable.h"

namespace oflux {
namespace lockfree {
namespace atomic {

// Note: template AtomicMapTrivial is already lock free
//       so no need to implement Trivial maps


template< typename K
	, typename A=AtomicExclusive >
class AtomicMapUnorderedWalker : public AtomicMapWalker {
public:
	AtomicMapUnorderedWalker(KVEnumeratorRef<K,A> & enumRef)
		: _enumRef(enumRef)
	{}
	virtual ~AtomicMapUnorderedWalker() {}
	virtual bool next(const void * & key,Atomic * &atom)
	{
		return _enumRef.next(key,atom);
	}
private:
	KVEnumeratorRef<K,A> _enumRef;
};

template< typename K
	, typename A=AtomicExclusive >
class AtomicMapUnordered : public AtomicMapAbstract {
public:
	typedef HashTable<K,A,16 /*for now*/ > Table;
	AtomicMapUnordered() {}
	virtual ~AtomicMapUnordered() {}

	virtual const void *get(Atomic * &atomic, const void * key)
	{
		K * k = reinterpret_cast<const K *>(k);
		const A * res = _table.get(*k);
		if(res == Table::HTC::Does_Not_Exist) {
			// insert it
			res = new A(NULL);
			const A * cas_res = _table.compareAndSwap(
				  *k
				, Table::HTC::Does_Not_Exist
				, res);
			if(Table::HTC::Does_Not_Exist != cas_res) { // cas succeeds
				res = _table.get(*k);
			}
		}
		atomic = res;
		return dunno; // FIXME
	}
	virtual int compare(const void * v_k1, const void * v_k2) const
	{ return atomic::compare<K>(v_k1,v_k2); }
	virtual void * new_key() const
	{ return new K(); }
	virtual void delete_key(void *o) const
	{ delete reinterpret_cast<K *>(o); }
	virtual AtomicMapWalker * walker()
	{ return new AtomicMapUnorderedWalker<K,A>(_table.getKeyValues()); }
private:
	Table _table;
};

} // namespace atomic
} // namespace lockfree
} // namespace oflux


#endif // OFLUX_LF_ATOMIC_MAPS
