#include "atomic/OFluxAtomicHolder.h"
#include "event/OFluxEventBase.h"
#include "flow/OFluxFlowNode.h"
#include <cassert>
#include <stdlib.h>
#include "OFluxLibDTrace.h"

#include "OFluxLogging.h"


namespace oflux {
namespace atomic {

EventBasePtr &
AtomicsHolder::no_event = EventBase::no_event;

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
AtomicsHolder::acquire_all_or_wait(
	  EventBasePtr & ev
	, EventBasePtr & pred_ev)
{
	AtomicsHolder & given_atomics =
		( pred_ev.get() 
		? pred_ev->atomics()
		: empty_ah);
	EventBase * ev_bptr = ev.get();
	const char * ev_name = ev_bptr->flow_node()->getName();
	_NODE_ACQUIREGUARDS(
		  ev_bptr
		, ev_name);
	const void * node_in = ev_bptr->input_type();
	if(!_is_sorted_and_keyed) {
		get_keys_sort(node_in);
	}
	assert(given_atomics._is_sorted_and_keyed
		|| given_atomics._number == 0);

	int blocking_index = -1;
	HeldAtomic * given_ha = NULL;
	HeldAtomic * my_ha = NULL;
	AtomicsHolderTraversal given_aht(given_atomics);
	AtomicsHolderTraversal my_aht(*this,_working_on);
	bool more_given = given_aht.next(given_ha);
	oflux_log_trace2("[%d] AH::aaow: %s %p needs %d atomics\n"
		, oflux_self()
		, ev_name
		, ev.get()
		, _number);
	while(blocking_index == -1 && my_aht.next(my_ha)) {
		my_ha->build(node_in,this,true);
		oflux_log_trace2("[%d] AH::aaow: %s given: %s %p %s%s my: %s %p %s%s  compare: %d\n"
			, oflux_self()
			, ev_name
			, more_given ? given_ha->atomic()->atomic_class() : "<null>"
			, more_given ? given_ha->atomic() : NULL
			, more_given && given_ha->haveit() ? "have it" : ""
			, more_given && given_ha->skipit() ? "skip it" : ""
			, my_ha->atomic()->atomic_class()
			, my_ha->atomic()
			, my_ha->haveit() ? "have it" : ""
			, my_ha->skipit() ? "skip it" : ""
			, more_given ? given_ha->compare(*my_ha) : -99);
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
			oflux_log_trace2("[%d] AH::aaow: takeit\n", oflux_self());
			_GUARD_ACQUIRE(
				  my_ha->flow_guard_ref()->getName().c_str()
				, ev_name
				, 1);
		} else {
			if(!my_ha->acquire_or_wait(ev,ev_name)) {
				// event is now queued
				// for waiting
				blocking_index = my_aht.index()-1; // next-1
				oflux_log_trace2("[%d] AH::aaow: wait\n", oflux_self());
			} else {
				oflux_log_trace2("[%d] AH::aaow: acquire\n", oflux_self());
			}
		}
	}
        _working_on = std::max(blocking_index,_working_on);
	if(blocking_index == -1) {
		_NODE_HAVEALLGUARDS(
			  static_cast<void *>(ev_bptr)
			, ev_name);
	}
	oflux_log_trace2("[%d] AH::aaow: return %d\n"
		, oflux_self()
		, blocking_index);
	return blocking_index == -1;
	// return true when all guards are acquired
}

void 
AtomicsHolder::release(
	  std::vector<EventBasePtr> & released_events
	, EventBasePtr & by_ev)
{
	// reverse order
	for(int i = _number-1; i >= 0; --i) {
		HeldAtomic * ha = get(i);
		assert(ha);
		if(ha->haveit()) {
			Atomic * a = ha->atomic();
			size_t pre_sz = released_events.size();
			a->release(released_events,by_ev);
			if(released_events.size() - pre_sz > 0) {
				oflux_log_trace2("[%d] AH::release %s released no events\n"
					, oflux_self()
					, by_ev->flow_node()->getName());
			}
			// acquisition happens here for released events
			bool should_relinquish = false;
			for(size_t k = pre_sz; k < released_events.size(); ++k) {
				should_relinquish = true;
				EventBasePtr & rel_ev = released_events[k];
				AtomicsHolder & rel_atomics = rel_ev->atomics();
				bool fd = false;
				HeldAtomic * rel_ha_ptr = NULL;
				oflux_log_trace2("[%d] AH::release %s released %s on recver atomic %p %s which is %s\n"
					, oflux_self()
					, by_ev->flow_node()->getName()
					, rel_ev->flow_node()->getName()
					, a
					, ha->haveit() ? "have it" : ""
					, ha->flow_guard_ref()->getName().c_str());
				_GUARD_RELEASE(ha->flow_guard_ref()->getName().c_str()
					, by_ev->flow_node()->getName() 
					, k);
				for(int j = rel_atomics.working_on()
						; j < rel_atomics.number()
						; ++j) {
					rel_ha_ptr = rel_atomics.get(j);
					oflux_log_trace2("[%d] AH::release                  sender atomic %p %s which is %s (compare %d)\n"
						, oflux_self()
						, rel_ha_ptr->atomic()
						, rel_ha_ptr->haveit() ? "have it" : ""
						, rel_ha_ptr->flow_guard_ref()->getName().c_str()
						, rel_ha_ptr->compare(*ha)
						);
					oflux_log_trace2("[%d] AH::release                  fd %d haveit %d pool_like %d\n"
						, oflux_self()
						, fd
						, rel_ha_ptr->haveit()
						, a->is_pool_like()
						);
					if(rel_ha_ptr && rel_ha_ptr->atomic() == a) {
						rel_ha_ptr->halftakeit(*ha);
						fd = true;
						if(j==rel_atomics.working_on()) {
							++rel_atomics._working_on;
						}
						_GUARD_ACQUIRE(
							  rel_ha_ptr->flow_guard_ref()->getName().c_str()
							, rel_ev->flow_node()->getName() 
							, 1);
						break;
					} else if(a->is_pool_like() 
							&& rel_ha_ptr
							&& (rel_ha_ptr->compare(*ha) == 0)) {
							//rel_ha_ptr->swap(*ha);
						//}
						rel_ha_ptr->halftakeit(*ha);
						fd = true;
						if(j==rel_atomics.working_on()) {
							++rel_atomics._working_on;
						}
						_GUARD_ACQUIRE(
							  rel_ha_ptr->flow_guard_ref()->getName().c_str()
							, rel_ev->flow_node()->getName() 
							, 1);
						break;
					}
				}
				assert(fd && "should have found a held atomic for this released event"); 
				assert( (rel_ha_ptr==NULL) || rel_ha_ptr->haveit());
			}
			if(should_relinquish) {
				ha->relinquish();
			}
                        ha->atomic(NULL);

#ifdef HAS_DTRACE
			size_t post_sz = released_events.size();
#endif // HAS_DTRACE
			_GUARD_RELEASE(ha->flow_guard_ref()->getName().c_str()
				, (post_sz > pre_sz 
					? released_events.back()->flow_node()->getName() 
					: "<nil>")
				, post_sz - pre_sz);
		}
	}
}

AtomicsHolder AtomicsHolder::empty_ah;


} // namespace atomic
} // namespace oflux
