#include "OFluxAtomicHolder.h"
#include "OFluxEvent.h"
#include <cassert>
#include <stdlib.h>
#include "OFluxLibDTrace.h"


namespace oflux {

void 
AtomicsHolder::add(flow::GuardReference * fg)
{
	assert(_number < MAX_ATOMICS_PER_NODE );
	_holders[_number].init(fg);
        int li = fg->getLexicalIndex(); 
        assert(li >= 0);
        _lexical[li] = &_holders[_number];
	_number++;
}

// it is assumed that the holder has the property that the guard_types are
// ascending
class AtomicsHolderTraversal {
public:
	AtomicsHolderTraversal(AtomicsHolder & ah, int startindex = 0)
		: _ah(ah)
		, _index(startindex)
		{}
	inline bool next(HeldAtomic * &);
	inline int index() const { return _index; }
private:
	AtomicsHolder & _ah;
	int             _index;
};

inline bool 
AtomicsHolderTraversal::next(HeldAtomic * & ha_ptr)
{
	bool res = false;
	if(_index >= _ah.number()) {
		res = false;
	} else {
		ha_ptr = _ah.get(_index);
		_index++;
		res = true;
	}
	return res;
}

typedef HeldAtomic * HeldAtomicPtr;

static int 
compare_held_atomics(const void * v_ha1,const void * v_ha2)
{
	const HeldAtomicPtr * ha1 = 
                reinterpret_cast<const HeldAtomicPtr *>(v_ha1);
	const HeldAtomicPtr * ha2 = 
                reinterpret_cast<const HeldAtomicPtr *>(v_ha2);
	return (*ha1)->compare(**ha2);
}

bool 
AtomicsHolder::get_keys_sort(const void * node_in)
{
	bool res = true;
	// get keys and atomics
	// init the sorted array
	for(int i = 0; i < _number; i++) {
		bool build_res = _holders[i].build(node_in,this);
		res = res && build_res;
		_sorted[i] = &(_holders[i]);
	}
	if(!_is_completely_sorted) {
                qsort(    _sorted
			, _number
			, sizeof(HeldAtomic *)
			, compare_held_atomics);
        }
	_is_sorted_and_keyed = true;
	return res;
}

int 
AtomicsHolder::acquire(
	  AtomicsHolder & given_atomics
	, const void * node_in
	, const char * for_event_name)
{
	if(!_is_sorted_and_keyed) {
		get_keys_sort(node_in);
	}
	assert(given_atomics._is_sorted_and_keyed
		|| given_atomics._number == 0);

	int result = -1;
	HeldAtomic * given_ha = NULL;
	HeldAtomic * my_ha = NULL;
	AtomicsHolderTraversal given_aht(given_atomics);
	AtomicsHolderTraversal my_aht(*this,_working_on);
	bool more_given = given_aht.next(given_ha);
	while(result == -1 && my_aht.next(my_ha)) {
		my_ha->build(node_in,this,true);
		while(more_given 
                                && (!given_ha->haveit() 
                                        || given_ha->compare(*my_ha) < 0)) {
			more_given = given_aht.next(given_ha);
		}
		if(my_ha->haveit() || my_ha->skipit()) {
			// nothing to do
		} else if(more_given 
                                && (!given_ha->skipit())
                                && given_ha->compare(*my_ha) == 0 
				&& given_ha->haveit()) {
			my_ha->takeit(*given_ha);
			_GUARD_ACQUIRE(
				  my_ha->flow_guard_ref()->getName().c_str()
				, for_event_name
				, 1);
		} else {
			if(!my_ha->acquire()) {
				result = my_aht.index()-1; // next-1
			} else {
				_GUARD_ACQUIRE(
					  my_ha->flow_guard_ref()->getName().c_str()
					, for_event_name
					, 0);
			}
		}
	}
        _working_on = std::max(result,_working_on);
	return result;
}

void 
AtomicsHolder::release(
	  std::vector<EventBasePtr > & released_events)
{
	// reverse order
	for(int i = _number-1; i >= 0; i--) {
		HeldAtomic * ha = get(i);
		assert(ha);
		if(ha->haveit()) {
			Atomic * a = ha->atomic();
#ifdef HAS_DTRACE
			int pre_sz = released_events.size();
#endif // HAS_DTRACE
			a->release(released_events);
                        ha->atomic(NULL);
#ifdef HAS_DTRACE
			int post_sz = released_events.size();
#endif // HAS_DTRACE
			_GUARD_RELEASE(ha->flow_guard_ref()->getName().c_str(),
				(post_sz > pre_sz ? released_events.back()->flow_node()->getName() : "<nil>"),
				post_sz - pre_sz);
		}
	}
}

AtomicsHolder AtomicsHolder::empty_ah;


}; // namespace
