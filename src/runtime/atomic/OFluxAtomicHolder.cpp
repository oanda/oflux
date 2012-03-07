#include "atomic/OFluxAtomicHolder.h"
#include "event/OFluxEventBase.h"
#include "flow/OFluxFlowNode.h"
#include <cassert>
#include <stdlib.h>
#include "OFluxLibDTrace.h"

#include "OFluxLogging.h"
#include "OFluxRollingLog.h"


namespace oflux {
namespace atomic {


EventBasePtr &
AtomicsHolder::no_event = EventBase::no_event;

#ifdef AH_INSTRUMENTATION
struct Observation {
	enum { max_at_index = 5 };
	enum Action {
		Action_none,
		Action_Acq,
		Action_acq,
		Action_Rel,
		Action_rel,
	} action;
	oflux_thread_t tid;
	EventBase * ev;
	const char * ev_name;
	struct {
		const char * name;
		int wtype;
		bool haveit;
		bool skipit;
		bool called;
		size_t released;
		EventBase * rel_ev;
		const char * rel_ev_name;

		void init() 
		{ name = ""; wtype = 0; haveit = false; 
		  skipit = false; called = false; 
		  released = 0; rel_ev = 0; rel_ev_name = name;
		}
	} atomics[max_at_index];
	int at_index;
	bool res;
	long long term_index;

	void init()
	{ action = Action_none; tid = 0; ev = 0; ev_name = "";
	  at_index = -1;
	  for(size_t i = 0; i < max_at_index; ++i) {
	    atomics[i].init();
	  }
	  res = false;
	  term_index = 0; }
};

RollingLog<Observation,256*4096> log;
#endif // AH_INSTRUMENTATION


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
	return (*ha1)->compare(**ha2,false);
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
	EventBase * ev_bptr = get_EventBasePtr(ev);
	EventBase * pred_ev_bptr = get_EventBasePtr(pred_ev);
#ifdef AH_INSTRUMENTATION
	Observation & obs = log.submit();
	obs.init();
	obs.tid = oflux_self();
	obs.ev = ev_bptr
	obs.ev_name = ev_bptr->flow_node()->getName();
	obs.action = Observation::Action_Acq;
#endif // AH_INSTRUMENTATION
	AtomicsHolder & given_atomics =
		( pred_ev_bptr
		? pred_ev_bptr->atomics()
		: empty_ah);
	const char * ev_name = ev_bptr->flow_node()->getName();
	PUBLIC_NODE_ACQUIREGUARDS(
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
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::aaow: %s %p needs %d atomics\n"
		, oflux_self()
		, ev_name
		, ev_b_ptr
		, _number);
	while(blocking_index == -1 && my_aht.next(my_ha)) {
		my_ha->build(node_in,this,true);
		oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::aaow: %s given: %s %p %s%s my: %s %p %s%s  compare: %d\n"
			, oflux_self()
			, ev_name
			, more_given && given_ha->atomic() 
			  ? given_ha->atomic()->atomic_class() 
			  : "<null>"
			, more_given ? given_ha->atomic() : NULL
			, more_given && given_ha->haveit() ? "have it" : ""
			, more_given && given_ha->skipit() ? "skip it" : ""
			, my_ha->atomic() 
			  ? my_ha->atomic()->atomic_class()
			  : "."
			, my_ha->atomic()
			, my_ha->haveit() ? "have it" : ""
			, my_ha->skipit() ? "skip it" : ""
			, more_given ? given_ha->compare(*my_ha, true) : -99);
		while(more_given 
                                && (!given_ha->haveit() 
                                        || given_ha->compare(*my_ha,true) < 0)) {
			more_given = given_aht.next(given_ha);
		}
#ifdef AH_INSTRUMENTATION
		obs.at_index = my_aht.index()-1;
		if(obs.at_index < Observation::max_at_index) {
			obs.atomics[obs.at_index].haveit = my_ha->haveit();
			obs.atomics[obs.at_index].skipit = my_ha->skipit();
			obs.atomics[obs.at_index].name = my_ha->flow_guard_ref()->getName().c_str();
			obs.atomics[obs.at_index].wtype = my_ha->flow_guard_ref()->wtype();
		}
#endif // AH_INSTRUMENTATION
		if(my_ha->haveit() || my_ha->skipit()) {
			// nothing to do
			_working_on = std::max(_working_on,my_aht.index()-1);
		/** no passing 
		} else if(more_given 
                                && (!given_ha->skipit())
                                && given_ha->compare(*my_ha) == 0 
				&& given_ha->haveit()) {
			my_ha->takeit(*given_ha);
			oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::aaow: takeit\n", oflux_self());
			_GUARD_ACQUIRE(
				  my_ha->flow_guard_ref()->getName().c_str()
				, ev_name
				, 1);
			_working_on = std::max(_working_on,my_aht.index()-1);
		**/
		} else {
#ifdef AH_INSTRUMENTATION
			if(obs.at_index < Observation::max_at_index) {
				obs.atomics[obs.at_index].called = true;
			}
#endif // AH_INSTRUMENTATION
			_working_on = std::max(my_aht.index()-1,_working_on);
			if(!my_ha->acquire_or_wait(ev,ev_name)) {
				// event is now queued
				// for waiting
				blocking_index = my_aht.index()-1; // next-1
				oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::aaow: wait\n", oflux_self());
			} else {
#ifdef AH_INSTRUMENTATION
				if(obs.at_index < Observation::max_at_index) {
					obs.atomics[obs.at_index].released = 1; // acquire note
				}
#endif // AH_INSTRUMENTATION
				oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::aaow: acquire\n", oflux_self());
			}
		}
	}
	//int __attribute__((unused)) working_on_local = _working_on;
        //_working_on = std::max(blocking_index,_working_on);
	//assert( (_working_on > 0 ? _sorted[_working_on-1]->haveit() : true) );
	// NOTE: can't access this thing after you fail to acquire
	//       since it is not your event (atomic holder) any more
	if(blocking_index == -1) {
		PUBLIC_NODE_HAVEALLGUARDS(
			  static_cast<void *>(ev_bptr)
			, ev_name);
	}
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::aaow: return %d\n"
		, oflux_self()
		, blocking_index);
#ifdef AH_INSTRUMENTATION
	obs.res = (blocking_index == -1);
	obs.action = Observation::Action_acq;
	obs.term_index = log.at();
	if(obs.res) {
		_full_acquire_time = gethrtime();
	}
#endif // AH_INSTRUMENTATION
	return blocking_index == -1;
	// return true when all guards are acquired
}

void 
AtomicsHolder::release(
	  std::vector<EventBasePtr> & released_events
	, EventBasePtr & by_ev)
{
	EventBase * by_ev_bptr = get_EventBasePtr(by_ev);
#ifdef AH_INSTRUMENTATION
	Observation & obs = log.submit();
	obs.init();
	obs.tid = oflux_self();
	obs.ev = by_ev_bptr;
	obs.ev_name = by_ev_bptr->flow_node()->getName();
	obs.action = Observation::Action_Rel;
	hrtime_t curr_hr_time = gethrtime();
	if(curr_hr_time - _full_acquire_time > 100000) {
		// threshold based log line at info level
		oflux_log_info("[" PTHREAD_PRINTF_FORMAT "] AH::release saw %s %p take %lld nsec from full acquire to release (held %d things: %d, %d, %d)\n"
			, oflux_self()
			, obs.ev_name
			, obs.ev
			, curr_hr_time - _full_acquire_time
			, _number
			, _holders[0].flow_guard_ref()
				? _holders[0].flow_guard_ref()->wtype()
				: 0
			, _holders[1].flow_guard_ref()
				? _holders[1].flow_guard_ref()->wtype()
				: 0
			, _holders[2].flow_guard_ref()
				? _holders[2].flow_guard_ref()->wtype()
				: 0
			);
	}
#endif // AH_INSTRUMENTATION
	// reverse order
	for(int i = _number-1; i >= 0; --i) {
		HeldAtomic * ha = get(i);
		assert(ha);
#ifdef AH_INSTRUMENTATION
		obs.at_index = i;
		if(i < Observation::max_at_index) {
			obs.atomics[i].name = ha->flow_guard_ref()->getName().c_str();
			obs.atomics[i].wtype = ha->flow_guard_ref()->wtype();
			obs.atomics[i].haveit = ha->haveit();
		}
#endif // AH_INSTRUMENTATION
		assert(ha->haveit() || ha->skipit());
		Atomic * a = ha->atomic();
		bool a_can_relinquish = false;
		if(ha->haveit() && a != NULL) {
			a_can_relinquish = a->can_relinquish();
			size_t pre_sz = released_events.size();
			a->release(released_events,by_ev);
			size_t post_sz = released_events.size();
			if(post_sz - pre_sz > 0) {
				oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AH::release %s %p released no events\n"
					, oflux_self()
					, by_ev_bptr->flow_node()->getName()
					, by_ev_bptr
					);
			}
#ifdef AH_INSTRUMENTATION
			obs.res = post_sz;
			if(i < Observation::max_at_index) {
				obs.atomics[i].called = true;
				obs.atomics[i].released = post_sz - pre_sz;
				if(obs.atomics[i].released) {
					obs.atomics[i].rel_ev = get_EventBasePtr(released_events[pre_sz]);
					obs.atomics[i].rel_ev_name = released_events[pre_sz]->flow_node()->getName();
				}
			}
#endif // AH_INSTRUMENTATION
			// acquisition happens here for released events
			bool should_relinquish = false;
			for(size_t k = pre_sz; k < post_sz; ++k) {
				should_relinquish = true;
				EventBasePtr & rel_ev = released_events[k];
				EventBase * rel_ev_bptr = get_EventBasePtr(rel_ev);
				AtomicsHolder & rel_atomics = rel_ev_bptr->atomics();
				bool fd = false;
				HeldAtomic * rel_ha_ptr = NULL;
				oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AH::release %s %p released %s %p on recver atomic %p %s which is %s\n"
					, oflux_self()
					, by_ev_bptr->flow_node()->getName()
					, by_ev_bptr
					, rel_ev_bptr->flow_node()->getName()
					, rel_ev_bptr
					, a
					, ha->haveit() ? "have it" : ""
					, ha->flow_guard_ref()->getName().c_str());
				PUBLIC_GUARD_RELEASE(ha->flow_guard_ref()->getName().c_str()
					, by_ev_bptr->flow_node()->getName() 
					, k);
				for(int j = rel_atomics.working_on()
						; j < rel_atomics.number()
						; ++j) {
					rel_ha_ptr = rel_atomics.get(j);
					oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::release                  sender atomic %p %s which is %s (compare %d)\n"
						, oflux_self()
						, rel_ha_ptr->atomic()
						, rel_ha_ptr->haveit() ? "have it" : ""
						, rel_ha_ptr->flow_guard_ref()->getName().c_str()
						, rel_ha_ptr->compare(*ha,true)
						);
					oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AH::release                  fd %d haveit %d pool_like %d\n"
						, oflux_self()
						, fd
						, rel_ha_ptr->haveit()
						, a->is_pool_like()
						);
					if(rel_ha_ptr 
						&& (rel_ha_ptr->atomic() == a
							|| (a->is_pool_like() && (rel_ha_ptr->compare(*ha, true) == 0)))
							) {
						rel_ha_ptr->halftakeit(*ha);
						fd = true;
						if(j==rel_atomics.working_on()) {
							rel_atomics._working_on = std::max(rel_atomics._working_on, j+1);
							assert( (_working_on > 0 
								? _sorted[_working_on-1]->haveit() || _sorted[_working_on-1]->skipit()
								: true) );
						}
						PUBLIC_GUARD_ACQUIRE(
							  rel_ha_ptr->flow_guard_ref()->getName().c_str()
							, rel_ev_bptr->flow_node()->getName() 
							, 1);
						break;
					}
				}
				(void)fd;
				assert(fd && "should have found a held atomic for this released event"); 
				assert( (rel_ha_ptr==NULL) || rel_ha_ptr->haveit());
			}

			if(post_sz == pre_sz && a->has_no_waiters() && a->held() == 0) {
 				ha->garbage_collect();
				if(!ha->atomic()) { a = NULL; }
 			}
			ha->atomic(NULL);
			PUBLIC_GUARD_RELEASE(ha->flow_guard_ref()->getName().c_str()
				, (post_sz > pre_sz 
					? released_events.back()->flow_node()->getName() 
					: "<nil>")
				, post_sz - pre_sz);
			if(a && a_can_relinquish) {
				a->relinquish(should_relinquish);
			}
		} else {
			oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AH::release skipping %d atomic for event %s %p since !haveit()\n"
				, oflux_self()
				, i
				, by_ev_bptr->flow_node()->getName()
				, by_ev_bptr
				);
		}
	}
#ifdef AH_INSTRUMENTATION
	obs.action = Observation::Action_rel;
	obs.term_index = log.at();
#endif // AH_INSTRUMENTATION
}

AtomicsHolder AtomicsHolder::empty_ah;


} // namespace atomic
} // namespace oflux
