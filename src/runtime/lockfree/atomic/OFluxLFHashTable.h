#ifndef OFLUX_LF_HASHTABLE_H
#define OFLUX_LF_HASHTABLE_H
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <cstddef>
#include "OFlux.h" // for the hash stuff
#include "lockfree/OFluxEnumerator.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "lockfree/OFluxDistributedCounter.h"

/**
 * @file OFluxLFHashTable.h
 * @author Mark Pichora
 *
 * This hash table is originally attributable to Cliff Click's
 * scalable concurrent hash table (he did it in Java).  What is 
 * here is a C++ port of Josh Lybnis' C port:
 *
 * Josh L:
 *    http://code.google.com/p/nbds/
 * Cliff Click:
 *    http://www.azulsystems.com/events/javaone_2008/2008_CodingNonBlock.pdf
 *
 * The was done by Cliff Click, the rest is just porting and testing.
 */



namespace oflux {
namespace lockfree {

#define MASK(n)           ((1ULL << (n)) - 1)
#define VOLATILE_DEREF(x) (*((volatile typeof(x))(x)))
#define VOLATILE_SELF     VOLATILE_DEREF(this)

class HashTableBase {
public:
	size_t _copies;
	size_t _probe;
	double _density;
};

template<bool checkbool>
class CompileCheck;

template<>
class CompileCheck<true> {};

template<>
class CompileCheck<false> {
public:
	CompileCheck(const char * c = "check failed");
};

template<typename V>
class HashTableConstants {
public:

	static const V * Does_Not_Exist;
	static const V * Copied_Value;
	static const V * TombStone;

	static const unsigned int TAG1 = 1U;
		//(1U << (sizeof(unsigned int)*8 -1));

	class Cas_Expect {
	public:
		static const V * Whatever;
		static const V * Exists;
		static const V * Does_Not_Exist;
	};

	static CompileCheck<sizeof(unsigned int) == sizeof(const V *)> cc_v_ptr_size;
	static inline const V * Tag_Value(const V * v) {
		return reinterpret_cast<const V*>((uintptr_t)v | TAG1);
	}
	static inline const V * Strip_Tag(const V * v) {
		return reinterpret_cast<const V*>((uintptr_t)v & ~TAG1);
	}
	static inline bool Is_Tagged(const V * v) {
		return (uintptr_t)v & TAG1;
	}
};

template<typename V>
const V * HashTableConstants<V>::Does_Not_Exist 
	= NULL;
template<typename V>
const V * HashTableConstants<V>::Copied_Value 
	= HashTableConstants<V>::Tag_Value(Does_Not_Exist);
template<typename V>
const V * HashTableConstants<V>::TombStone 
	= Strip_Tag((const V *)-1);
template<typename V>
const V * HashTableConstants<V>::Cas_Expect::Whatever 
	= (const V *)0;
template<typename V>
const V * HashTableConstants<V>::Cas_Expect::Exists 
	= (const V *)-1;
template<typename V>
const V * HashTableConstants<V>::Cas_Expect::Does_Not_Exist
	= (const V *)-2; 

// forward declaration
template< typename K
	, typename V >
class HashTable;

// forward declaration
template< typename K
	, typename V 
	, typename EB >
class HashTableEnumeratorBase;

// forward declaration
template< typename K
	, typename V >
class KeyValueHashTableEnumerator;

template< typename K
	, typename V >
class HashTableImplementation {
public:
	friend class HashTable<K,V>;
	friend class HashTableEnumeratorBase<K,V,KVEnumeratorRef<K,V> >;
	friend class HashTableEnumeratorBase<K,V,EnumeratorRef<K> >;

	typedef HashTableConstants<V> HTC;
	/**
	 * @brief hold the entries (key-value pairs of the table)
	 */
	struct Entry {
		Entry() : key(NULL) , value(NULL) {}

		K * key; // use the has_ptr template
		V * value;
	};

	/*static const K * key_from_value(const V *v) {
		static const Entry e;
		static const size_t offset = reinterpret_cast<size_t>(&e.value)
			- reinterpret_cast<size_t>(&e);
		return reinterpret_cast<const K *>(reinterpret_cast<const char *>(v)-offset);
	}*/

	/**
	 * @brief hold the constants that are needed by the implementation
	 */
	enum    //                          presumed log-size of a cache line
		{ Cache_Line_Scale          = MachineSpecific::Cache_Line_Scale
		//                          presumed size of a cache line
		, Cache_Line_Size           = MachineSpecific::Cache_Line_Size
		//                          number of entries in each bucket
		, Entries_Per_Bucket        = Cache_Line_Size/sizeof(Entry)
		//                          number of copyEntry()s per helpCopy
		, Entries_Per_Copy_Chunk    = Cache_Line_Size/sizeof(Entry)
		//                          log-initial-size of the hashtable
		, Min_Scale                 = 4
		};

	/**
	 * @brief count (per thread) statistics need by the implementation
	 */
	struct Stats {
		Stats() 
			: count(0)
			, num_entries_copied(0) 
		{}

		size_t count;
		Counter<size_t> key_count;
		Counter<size_t> copy_scan;
		size_t num_entries_copied;
	};


	HashTableImplementation(
			  HashTableBase & htb
			, unsigned short scale = Min_Scale)
		: _scale(scale)
		, _htb(htb) 
		, _table(NULL)
		, _next(NULL)
		, _probe(_scale * 1.5 + 2)
		, _ref_count(1)
	{
		size_t quarter = (1ULL << (_scale -2)) / Entries_Per_Bucket;
		if(_probe  > quarter && quarter > 4) {
			_probe = quarter;
		}
		assert(_probe);
		size_t sz = (1ULL << _scale);
		_table = new Entry[sz];
		assert(_scale >= Min_Scale && _scale < 63); // size must be a power of 2
		assert(sizeof(Entry) * Entries_Per_Bucket % Cache_Line_Size == 0); // divisible into cache
		//assert((size_t)_table % Cache_Line_Size == 0); // cache aligned
	}

	~HashTableImplementation() {
		HashTableEnumeratorBase<K,V,KVEnumeratorRef<K,V> > enumr(this,true);
		const K * kp;
		const V * vp;
		while(enumr.next(kp,vp)) {
			if(kp) {
				delete kp;
				kp = NULL;
			}
			if(vp && vp != HTC::TombStone && vp != HTC::Copied_Value ) {
				delete vp;
				vp = NULL;
			}
		}
		if(_table) { 
			delete [] _table;
			_table = NULL;
		}
		if(_next) {
			delete _next;
			_next = NULL;
		}
	}


	// get() _______________________________________________________
	// Get the value associated with the given key
	// Return:
	// 1. Does_Not_Exist if it is not in the table
	// 2. otherwise, the value pointer for that key
	// If a copy is underway, this function will help copyEntry()
	const V *
	get(const K & k, size_t k_hash) {
		bool is_empty;
		volatile Entry * ent = lookUp(k,k_hash,is_empty);
		if(ent == NULL) {
			if(VOLATILE_SELF._next != NULL) {
				return _next->get(k,k_hash);
			}
			return HTC::Does_Not_Exist;
		}
		if(is_empty) {
			return HTC::Does_Not_Exist;
		}
		V * ent_val = ent->value;
		if(HTC::Is_Tagged(ent_val)) {
			if(ent_val != HTC::Copied_Value
					&& ent_val != HTC::Tag_Value(HTC::TombStone)) {
				bool did_copy = copyEntry(
					  ent
					, k_hash
					, VOLATILE_SELF._next
					);
				if(did_copy) {
					__sync_fetch_and_add(&_stats.num_entries_copied,1);
				}
			}
			return VOLATILE_SELF._next->get(k,k_hash);
		}
		return (ent_val == HTC::TombStone 
			? HTC::Does_Not_Exist
			: ent_val);
	}
			
	bool
	remove_key(const K & k, size_t k_hash)
	{
		bool is_empty;
		volatile Entry * ent = lookUp(k,k_hash,is_empty);
		K * ent_k = ent->key;
		if(!is_empty 
				&& ent->value == HTC::TombStone 
				&& __sync_bool_compare_and_swap(
					  &ent->key
					, ent_k
					, NULL)) {
			if(ent->value == HTC::TombStone) {
				//printf("%d ht rmk del k %p\n",pthread_self(), ent_k);
				delete ent_k;
				__sync_bool_compare_and_swap(&ent->value,HTC::TombStone,NULL);
				return true;
			}
		}
		return false;
	}

protected:

	// compareAndSwap() ___________________________________________
	// Find the value associated with k
	// Return value will match expected_val when replacement is a success
	// Some cases:
	// 1. normal values:
	//    If it matches expected_val then replace it with new_val
	// 2. new_val == Does_Not_Exist 
	//    replace value with a TombStone
	// 3. expected_val == Cas_Expect::Exists
	//    matches any normal expected value (replacement happens)
	//    as long as k is in the table
	// 4. expected_val == Cas_Expect::Does_Not_Exist
	//    insert happens as long as there was no k in the table
	// 5. expected_val == Cas_Expect::Whatever
	//    ... ??
	// Return:
	// 1. old value associated with k
	// 2. Does_Not_Exist if k was not in the table
	// 3. Copied_Value indicates that a new table was started 
	//    caller seems to need a retry?
	const V * 
	compareAndSwap(  const K & k
			, const V * expected_val
			, const V * new_val
			, size_t k_hash) {
		assert(!HTC::Is_Tagged(new_val));
		bool is_empty;
		volatile Entry * ent = lookUp(k,k_hash,is_empty);
		if(ent == NULL) {
			if(_next == NULL) {
				startCopy();
			}
			return HTC::Copied_Value;
		}
		if(is_empty) {
			if( expected_val != HTC::Cas_Expect::Does_Not_Exist
					&& expected_val != HTC::Cas_Expect::Whatever) {
				return HTC::Does_Not_Exist;
			}
			if(new_val == HTC::Does_Not_Exist) {
				return HTC::Does_Not_Exist;
			}
			K * new_k = new K(k);
			//printf("%d ht cas this %p new k %p kh\n"
				//, pthread_self()
				//, this
				//, new_k
				//, k_hash);
			K * old_k = __sync_val_compare_and_swap(
					  &ent->key
					, NULL
					, new_k);
			if(old_k != NULL) {
				//printf("%d ht cas del k %p\n",pthread_self(),new_k);
				delete new_k;
				return compareAndSwap(
					  k
					, expected_val
					, new_val
					, k_hash);
			}
			++_stats.key_count;
		}
		V * ent_val = ent->value;
		if(HTC::Is_Tagged(ent_val)) {
			if(ent_val != HTC::Copied_Value
					&& ent_val != HTC::Tag_Value(HTC::TombStone)) {
				bool did_copy = copyEntry(
					  ent
					, k_hash
					, VOLATILE_SELF._next);
				if(did_copy) {
					__sync_fetch_and_add(&_stats.num_entries_copied,1);
				}
			}
			return HTC::Copied_Value;
		}
		bool old_existed = ent_val != HTC::TombStone
				&& ent_val != HTC::Does_Not_Exist;
		if(expected_val != HTC::Cas_Expect::Whatever
				&& expected_val != ent_val) {
			if(expected_val != (old_existed 
					? HTC::Cas_Expect::Exists
					: HTC::Cas_Expect::Does_Not_Exist)) {
				return ent_val;
			}
		}
		if((new_val == HTC::Does_Not_Exist && !old_existed)
				|| ent_val == new_val) {
			return ent_val;
		}
		V * val = __sync_val_compare_and_swap(
			  &ent->value
			, ent_val
			, (new_val == HTC::Does_Not_Exist
				? HTC::TombStone
				: new_val));
		if(val != ent_val) {
			return compareAndSwap(
				  k
				, expected_val
				, new_val
				, k_hash);
		}
		if(old_existed && new_val == HTC::Does_Not_Exist) {
			__sync_add_and_fetch(&_stats.count,-1);
		} else if(!old_existed && new_val != HTC::Does_Not_Exist) {
			__sync_add_and_fetch(&_stats.count,1);
		}
		return ent_val;
	}


	// lookUp() ____________________________________________________
	// Look in this implementation for a particular keyed entry.
	// - is_empty is provided as an optimization to allow the caller
	//   to avoid a key compare.
	// Q1: how often does its caller really go investigate _next?
	volatile Entry * 
	lookUp(const K & k, size_t k_hash, bool & is_empty) {
		is_empty = false;
		size_t index = k_hash & MASK(_scale);
		for(size_t i = 0; i < _probe; ++i) {
			volatile Entry * bucket = _table 
				+ (index & ~(Entries_Per_Bucket-1));
			for(size_t j = 0; j < Entries_Per_Bucket; ++j) {
				volatile Entry * ent = bucket
					+ ((index + j) & (Entries_Per_Bucket-1));
				const K * ent_k = ent->key;
				if(ent_k == NULL) {
					is_empty = true;
					// return with val != NULL and
					// is_empty = true indicating that
					// we hit an empty slot
					return ent;
				}
				if(k==*ent_k) { // real key compare
					// we found an entry with a 
					// real key that matches.
					return ent;
				}
			}
			index = nextIndex(index,k_hash);
		}
		// return with val = NULL, is_empty = false
		// indicating that there was no room for this entry here
		// and the caller should look to _next to see if it is 
		// in there.
		return NULL;
	}

	const K *
	getPersistentKey( const K * k, size_t k_hash )
	{
		bool is_empty;
		volatile Entry * ent = lookUp(
			  *k
			, k_hash
			, is_empty);
		if(_next && ent == NULL) {
			return VOLATILE_SELF._next->getPersistentKey(k,k_hash);
		}
		return is_empty || (ent == NULL) ? NULL : ent->key;
	}

	// nextIndex() _________________________________________________
	// indicate the next position in the raw hashtable to look for 
	// a particular element (hash given)
	inline size_t 
	nextIndex(size_t old_index, size_t k_hash) {
		size_t incr = (k_hash >> (32 - _scale));
		if(incr < Entries_Per_Bucket) {
			incr += Entries_Per_Bucket;
		}
		return (old_index + incr) & MASK(_scale);
	}


	// startCopy() _________________________________________________
	// initiate a copy by creating a new _next table
	void 
	startCopy() {
		size_t count = _stats.count;
		unsigned short newscale = _scale;
		newscale += (count > (1ULL << (_scale - 1))) 
			|| (count > (1ULL << (_scale - 2)) 
				+ (1ULL << (_scale - 3)));
		HashTableImplementation<K,V> * next =
			new HashTableImplementation<K,V>(
				  _htb
				, newscale);
		HashTableImplementation<K,V> * old_next =
			__sync_val_compare_and_swap(
				  &_next
				, NULL
				, next);
		if(old_next != NULL) {
			delete next;
		}
		__sync_fetch_and_add(&_htb._copies,1);
		_htb._density = (double)_stats.key_count.value() 
			/ (1ULL << _scale) * 100;
		_htb._probe = _probe;
	}

	// copyEntry() _________________________________________________
	// Copy a single Entry from this table to the _next table
	bool
	copyEntry(volatile Entry * ent // entry from hash table 1 to move into 2
		, size_t k_hash 
		, HashTableImplementation<K,V> * nht // hash table 2
		) {
		assert(_next);
		assert(nht);
		assert(ent >= _table && ent < _table + (1ULL << _scale));
		const V * ent_val = ent->value;
		if(ent_val == HTC::Copied_Value 
				|| ent_val == HTC::Tag_Value(HTC::TombStone)) {
			return false;
		} else if(ent_val == HTC::Does_Not_Exist) {
			ent_val = __sync_val_compare_and_swap(
					  &ent->value
					, HTC::Does_Not_Exist
					, HTC::Copied_Value);
			if(ent_val == HTC::Does_Not_Exist) {
				return true;
			}
		}
		ent_val = __sync_fetch_and_or(
			  &ent->value
			, HTC::Tag_Value(0));
		if(ent_val == HTC::Copied_Value
				|| ent_val == HTC::Tag_Value(HTC::TombStone)) {
			return false;
		}
		if(ent_val == HTC::TombStone) {
			return true;
		}
		K * ent_key = ent->key;
		if(k_hash == 0) {
			k_hash = hash_ptr<K>()(ent_key);
		}
		bool nht_ent_is_empty;
		volatile Entry * nht_ent = nht->lookUp(
			  *ent_key
			, k_hash
			, nht_ent_is_empty);
		if(nht_ent == NULL) {
			if(nht->_next == NULL) {
				nht->startCopy();
			}
			return copyEntry(ent, k_hash, nht->_next);
		}
		if(nht_ent_is_empty) {
			K * old_nht_ent_k = __sync_val_compare_and_swap(
				  &nht_ent->key
				, HTC::Does_Not_Exist
				, ent_key);
			if(old_nht_ent_k != NULL) {
				return copyEntry(ent,k_hash,nht);
			}
			nht->_stats.key_count++;
		}
		ent_val = HTC::Strip_Tag(ent_val);
		V * old_nht_ent_val = __sync_val_compare_and_swap(
			  &nht_ent->value
			, HTC::Does_Not_Exist
			, ent_val);
		if(old_nht_ent_val == HTC::Copied_Value) {
			return copyEntry(ent,k_hash,nht->_next);
		}
		ent->value = const_cast<V *>(HTC::Copied_Value);
		if(old_nht_ent_val == HTC::Does_Not_Exist) {
			__sync_add_and_fetch(&_stats.count,-1);
			__sync_add_and_fetch(&nht->_stats.count,+1);
			return true;
		}
		return false;
	}

	// helpCopy() __________________________________________________
	// contribute some cycles to copying this table to its _next instance
	bool
	helpCopy() {
		volatile Entry * ent;
		size_t limit;
		size_t total_copied = _stats.num_entries_copied;
		size_t num_copied = 0;
		size_t x = _stats.copy_scan.value();
		if(total_copied != (1ULL << _scale)) {
			bool panic = (x >= (1ULL << (_scale +1)));
			if(!panic) {
				limit = Entries_Per_Copy_Chunk;
				_stats.copy_scan += Entries_Per_Copy_Chunk;
				ent = _table + (x & MASK(_scale));
			} else {
				ent = _table;
				limit = (1ULL << _scale);
			}
			// copy entries
			for(size_t i = 0; i < limit; ++i) {
				num_copied += copyEntry(ent++,0,_next);
				assert(ent <= _table + (1ULL << _scale));
			}
			if(num_copied) {
				total_copied = __sync_add_and_fetch(&_stats.num_entries_copied,num_copied);
			}
		}
		return (total_copied == (1ULL << _scale));
	}


	// release() ___________________________________________________
	// decrement the reference count, and return its new value
	int 
	release() {
		assert(_ref_count > 0);
		_ref_count = __sync_fetch_and_add(&_ref_count,-1);
		return _ref_count;
	}

	// count() _____________________________________________________
	// return the stats entry count
	inline size_t
	count()
	{ return _stats.count; }

private:
	unsigned short                 _scale;
	HashTableBase &                _htb;
	volatile Entry *               _table;
protected:
	HashTableImplementation<K,V> * _next;
	Stats                          _stats;
	size_t                         _probe;
	int                            _ref_count;
};

template< typename K
	, typename V 
	, typename EB >
class HashTableEnumeratorBase : public EB::UnderEnumerator {
public:
	typedef HashTableImplementation<K,V> Implementation;
	typedef HashTableConstants<V> HTC;

	HashTableEnumeratorBase(
			  Implementation * impl
			, bool localimplonly = false) 
		: _impl(impl)
		, _index(-1)
		, _table_size(1ULL << _impl->_scale)
	{
		int ref_count;
		do {
			while(!localimplonly && _impl->_next != NULL) {
				do { } while(!_impl->helpCopy());
				_impl = _impl->_next;
			}
			do {
				ref_count = _impl->_ref_count;
				if(ref_count == 0) {
					break;
				}
			} while( ref_count != 
					__sync_val_compare_and_swap(
						  &(_impl->_ref_count)
						, ref_count
						, ref_count+1));
		} while(ref_count == 0);
	}
	~HashTableEnumeratorBase() {
		_impl->release();
	}
	virtual bool next(const K * & k, const V * & v) {
		volatile typename Implementation::Entry * ent;
		do {
			_index++;
			if(_index >= (int)_table_size) {
				return false;
			}
			ent = &(_impl->_table[_index]);
			k = ent->key;
			v = ent->value;
		} while( k == NULL 
			|| v == HTC::Does_Not_Exist 
			|| v == HTC::TombStone);
		if(v == HTC::Copied_Value) {
			size_t h = hash_ptr<K>()(k);
			v = _impl->_next->get(*k,h);
			if(v == HTC::Does_Not_Exist) {
				return next(k,v);
			}
		}
		return k != NULL;
	}
protected:
	Implementation * _impl;
	int              _index; // signed
	size_t           _table_size;
};

template< typename K
	, typename V >
class KeyValueHashTableEnumerator : public HashTableEnumeratorBase<K,V,KVEnumeratorRef<K,V> > {
public:
	KeyValueHashTableEnumerator(HashTableImplementation<K,V> * impl) 
		: HashTableEnumeratorBase<K,V,KVEnumeratorRef<K,V> >(impl)
	{}
};

template< typename K
	, typename V >
class KeyHashTableEnumerator : public HashTableEnumeratorBase<K,V,EnumeratorRef<K> > {
public:

	KeyHashTableEnumerator(HashTableImplementation<K,V> * impl) 
		: HashTableEnumeratorBase<K,V,EnumeratorRef<K> >(impl)
	{}
	virtual bool next(const K * & kp) {
		const V * vp = NULL;
		return next(kp,vp);
	}
};

template< typename K
	, typename V >
class HashTable : public HashTableBase {
public:
	typedef HashTableImplementation<K,V> Implementation;
	typedef HashTableConstants<V> HTC;

	HashTable()
		: _impl(new Implementation(*this))
	{}

	~HashTable() {
		if(_impl) {
			delete _impl;
			_impl = NULL;
		}
	}

	// size() ______________________________________________________
	// Report the number of elements currently in the table
	// Q:will double count elements that are moving from one to another?
	inline size_t
	count() {
		size_t res = 0;
		Implementation * impl = _impl;
		while(impl) {
			res += impl->count();
		}
		return res;
	}

	// get() _______________________________________________________
	//
	inline const V *
	get(const K & k) {
		return _impl->get(k,hash<K>()(k));
	}

	// getPersistentKey ____________________________________________
	//  gets us the internal key when the get() return value is
	//  a real value
	inline const K *
	getPersistentKey(const K * k, size_t k_hash) {
		return _impl->getPersistentKey(k,k_hash);
	}

	// remove() ____________________________________________________
	// Remove an entry an return its value
	inline const V *
	remove(const K & k) {
		const V * val;
		size_t k_hash = hash<K>()(k);
		//printf("%d remove on kh %u\n", pthread_self(), k_hash);
		Implementation * impl = _impl;
		do {
			val = impl->compareAndSwap(
				  k
				, HTC::Cas_Expect::Whatever
				, HTC::Does_Not_Exist
				, k_hash);
			if(val != HTC::Copied_Value) {
				return val == HTC::TombStone
					? HTC::Does_Not_Exist
					: val;
			}
			assert(impl->_next);
			impl = impl->_next;
		} while(1);
		// now recover the key:
		impl->remove_key(k,k_hash);
	}

	// compareAndSwap() ____________________________________________
	//
	inline const V *
	compareAndSwap(   const K & k
			, const V * expected_val
			, const V * new_val) {
		assert(!HTC::Is_Tagged(new_val) 
			&& new_val != HTC::Does_Not_Exist
			&& new_val != HTC::TombStone);
		Implementation * impl = _impl;
		if(impl->_next != NULL) {
			bool done = impl->helpCopy();
			if(done) { // unlink a fully copieed table
				assert(impl->_next);
				if(__sync_bool_compare_and_swap(
						  &_impl
						, impl
						, impl->_next)) {
					//printf("%d impl up %p %p\n", pthread_self(), impl, impl->_next);
					Implementation * impl_next = impl->_next;
					if(0 == impl->release()) {
						delete impl;
					}
					impl = impl_next;
				}
			}
		}
		const V * old_val = NULL;
		size_t k_hash = hash<K>()(k);
		while(impl && (old_val = impl->compareAndSwap(
				  k
				, expected_val
				, new_val
				, k_hash)) == HTC::Copied_Value) {
			assert(impl->_next);
			impl = impl->_next;
		}
		return old_val == HTC::TombStone
			? HTC::Does_Not_Exist
			: old_val;
	}

	//EnumeratorRef<K>
	KeyHashTableEnumerator<K,V> *
	getKeys() {
		//return EnumeratorRef<K>(
		return new KeyHashTableEnumerator<K,V>(_impl);
		//);
	}

	//KVEnumeratorRef<K,V>
	KeyValueHashTableEnumerator<K,V> *
	getKeyValues() {
		//return KVEnumeratorRef<K,V>(
		return
			new KeyValueHashTableEnumerator<K,V>(_impl);
		//);
	}
private:
	Implementation * _impl;
};

} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LF_HASHTABLE_H
