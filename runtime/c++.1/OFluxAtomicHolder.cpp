#include "OFluxAtomicHolder.h"
#include "OFluxEvent.h"
#include <cassert>
#include <stdlib.h>

namespace oflux {
/*
HeldAtomic & HeldAtomic::operator=(const HeldAtomic & ha)
{
	_haveit = ha.haveit;
	_atom = ha._atom;
	_key = ha._key;
	_flow_guard = ha._flow_guard;
}
*/


void AtomicsHolder::add(FlowGuardReference * fg)
{
	assert(_number < MAX_ATOMICS_PER_NODE );
	_holders[_number].init(fg);
	_number++;
}

// it is assumed that the holder has the property that the guard_types are
// ascending
class AtomicsHolderTraversal {
public:
	AtomicsHolderTraversal(AtomicsHolder & ah)
		: _ah(ah)
		, _index(0)
		{}
	bool next(HeldAtomic * &);
	int index() const { return _index; }
private:
	AtomicsHolder & _ah;
	int             _index;
};

bool AtomicsHolderTraversal::next(HeldAtomic * & ha_ptr)
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

static int compare_held_atomics(const void * v_ha1,const void * v_ha2)
{
	const HeldAtomicPtr * ha1 = reinterpret_cast<const HeldAtomicPtr *>(v_ha1);
	const HeldAtomicPtr * ha2 = reinterpret_cast<const HeldAtomicPtr *>(v_ha2);
	return (*ha1)->compare(**ha2);
}

void AtomicsHolder::get_keys_sort(const void * node_in)
{
	// get keys and atomics
	// init the sorted array
	for(int i = 0; i < _number; i++) {
		_holders[i].build(node_in);
		_sorted[i] = &(_holders[i]);
	}
	qsort(_sorted,_number,sizeof(HeldAtomic *),compare_held_atomics);
	_is_sorted_and_keyed = true;
}

int AtomicsHolder::acquire(AtomicsHolder & given_atomics, 
	const void * node_in,
	const char * for_event_name)
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
	AtomicsHolderTraversal my_aht(*this);
	bool more_given = given_aht.next(given_ha);
	while(result == -1 && my_aht.next(my_ha)) {
		while(more_given && given_ha->compare(*my_ha) < 0) {
			more_given = given_aht.next(given_ha);
		}
		if(my_ha->haveit()) {
			// nothing to do
		} else if(more_given && given_ha->compare(*my_ha) == 0 
				&& given_ha->haveit()) {
			my_ha->takeit(*given_ha);
			_GUARD_ACQUIRE(my_ha->flow_guard_ref()->getName().c_str(),
				for_event_name,1);
		} else {
			if(!my_ha->acquire()) {
				result = my_aht.index()-1; // next-1
			} else {
				_GUARD_ACQUIRE(my_ha->flow_guard_ref()->getName().c_str(),
					for_event_name,0);
			}
		}
	}
	return result;
}

void AtomicsHolder::release( std::vector<boost::shared_ptr<EventBase> > & released_events)
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

}; // namespace
