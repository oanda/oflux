#include "OFluxEventOperations.h"
#include "OFluxEventBase.h"
#include "OFluxLibDTrace.h"
#include "OFluxFlowCase.h"
#include "OFluxFlowGuard.h"
#include "OFluxFlowNode.h"
#include "OFluxFlow.h"
#include "OFluxAtomicHolder.h"
#include "OFluxLogging.h"


namespace oflux {
namespace event {

int
acquire_guards(
	  EventBasePtr & ev
	, AtomicsHolder & ah)
{
	enum { AGR_Success = 1, AGR_MustWait = 0 };
	int res = AGR_Success;
	EventBase * ev_bptr = ev.get();
	const char * ev_name = ev_bptr->flow_node()->getName();
        _NODE_ACQUIREGUARDS(
		  static_cast<void *>(ev_bptr)
		, ev_name);
	AtomicsHolder & atomics = ev_bptr->atomics();
	int ah_res = atomics.acquire(
		  ah
		, ev_bptr->input_type()
		, ev_name);
	HeldAtomic * held_atomic = 
		( ah_res == -1 
		? NULL
		: atomics.get(ah_res) );
	int wtype = (held_atomic ? held_atomic->wtype() : 0);
	flow::GuardReference * flow_guard_ref __attribute__((unused)) = 
		(held_atomic ? held_atomic->flow_guard_ref() : NULL);
	Atomic * must_wait_on_atomic = 
		(held_atomic ? held_atomic->atomic() : NULL);
	if(must_wait_on_atomic) {
		res = AGR_MustWait;
		must_wait_on_atomic->wait(ev,wtype);
		_GUARD_WAIT(
			  flow_guard_ref->getName().c_str()
			, ev_name
			, wtype);
	} else {
                _NODE_HAVEALLGUARDS(
			  static_cast<void *>(ev_bptr)
			, ev_name);
        }
	return res;
}

AtomicsHolder & empty_ah = AtomicsHolder::empty_ah;

void
successors_on_no_error(
	  std::vector<EventBasePtr> & successor_events
	, EventBasePtr & ev)
{
	enum { return_code = 0 };
	std::vector<flow::Case *> fsuccessors;
	OutputWalker ev_ow = ev->output_type();
	void * ev_output = NULL;
	bool saw_source = false;
	while((ev_output = ev_ow.next()) != NULL) {
		fsuccessors.clear();
		ev->flow_node()->get_successors(
			  fsuccessors
			, ev_output
			, return_code);
		for(int i = 0; i < (int) fsuccessors.size(); i++) {
			flow::Node * fn = fsuccessors[i]->targetNode();
			flow::IOConverter * iocon = fsuccessors[i]->ioConverter();
			bool is_source = fn->getIsSource();
			if(is_source && saw_source) {
				continue;
				// only one source allowed
				// even with splaying
			}
			saw_source = saw_source || is_source;
			CreateNodeFn createfn = fn->getCreateFn();
			EventBasePtr ev_succ = 
				( is_source
				? (*createfn)(EventBase::no_event,NULL,fn)
				: (*createfn)(ev,iocon->convert(ev_output),fn)
				);
			ev_succ->error_code(0);
			if(event::acquire_guards(ev_succ
					, is_source
					? empty_ah
					: ev->atomics())) {
				successor_events.push_back(ev_succ);
			}
		}
	}
}

void
successors_on_error(
	  std::vector<EventBasePtr> & successor_events
	, EventBasePtr & ev
	, int return_code)
{
	std::vector<flow::Case *> fsuccessors;
	void * ev_output = ev->output_type().next();
	ev->flow_node()->get_successors(fsuccessors, 
			ev_output, return_code);
	for(int i = 0; i < (int) fsuccessors.size(); i++) {
		flow::Node * fn = fsuccessors[i]->targetNode();
		flow::IOConverter * iocon = fsuccessors[i]->ioConverter();
		CreateNodeFn createfn = fn->getCreateFn();
		bool was_source = ev->flow_node()->getIsSource();
		EventBasePtr ev_succ = 
			( was_source
			? (*createfn)(EventBase::no_event,NULL,fn)
			: (*createfn)(ev->get_predecessor(),iocon->convert(ev->input_type()),fn));
		ev_succ->error_code(return_code);
		if(event::acquire_guards(ev_succ,ev->atomics())) {
			successor_events.push_back(ev_succ);
		}
	}
}

void
push_initials_and_sources( 
	  std::vector<EventBasePtr> & events_vec
	, flow::Flow * flow)
{
	std::vector<flow::Node *> & sources = flow->sources();
	for(int i = 0; i < (int) sources.size(); ++i) {
		flow::Node * fn = sources[i];
                oflux_log_info("load_flow pushing %s\n",fn->getName());
		CreateNodeFn createfn = fn->getCreateFn();
		EventBasePtr ev = (*createfn)(EventBase::no_event,NULL,fn);
		if(event::acquire_guards(ev)) {
			events_vec.push_back(ev);
		}
	}
}

} // namespace event
} // namespace oflux
