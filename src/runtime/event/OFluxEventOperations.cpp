#include "event/OFluxEventOperations.h"
#include "event/OFluxEventBase.h"
#include "OFluxLibDTrace.h"
#include "flow/OFluxFlowCase.h"
#include "flow/OFluxFlowGuard.h"
#include "flow/OFluxFlowNode.h"
#include "flow/OFluxFlow.h"
#include "atomic/OFluxAtomicHolder.h"
#include "OFluxLogging.h"


namespace oflux {
namespace event {

inline int
__acquire_guards(
	  EventBasePtr & ev
	, EventBasePtr & pred_ev = EventBase::no_event)
{
	int __attribute__((unused)) working_on_local = ev->atomics().working_on();
	bool res = ev->atomics().acquire_all_or_wait(
		  ev
		, pred_ev);
	if(!res) {
		oflux_log_trace2("[%d] event::acquire_guards() failure for "
			"%s %p on guards acquisition\n"
			, oflux_self()
			, ev->flow_node()->getName()
			, ev.get());
	}
	return res;
}

int
acquire_guards(
	  EventBasePtr & ev
	, EventBasePtr & pred_ev)
{
	return __acquire_guards(ev,pred_ev);
}

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
			if(event::__acquire_guards(ev_succ
					, is_source
					? EventBase::no_event
					: ev)) {
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
		if(event::__acquire_guards(ev_succ,ev)) {
			successor_events.push_back(ev_succ);
		}
	}
}

void
push_initials_and_sources( 
	  std::vector<EventBasePtr> & events_vec
	, flow::Flow * flow
	, bool lifo)
{
	std::vector<flow::Node *> & sources = flow->sources();
	std::vector<EventBasePtr> events_backend;
	for(size_t i = 0; i < sources.size(); ++i) {
		flow::Node * fn = sources[i];
		if(fn->getIsInitial()) {
			oflux_log_info("load_flow pushing initial %s\n"
				, fn->getName());
			CreateNodeFn createfn = fn->getCreateFn();
			EventBasePtr ev = (*createfn)(EventBase::no_event,NULL,fn);
			if(event::__acquire_guards(ev)) {
				if(lifo) {
					events_vec.push_back(ev);
				} else {
					events_backend.push_back(ev);
				}
			}
		}
	}
	for(size_t i = 0; i < sources.size(); ++i) {
		flow::Node * fn = sources[i];
		if(!fn->getIsInitial()) {
			oflux_log_info("load_flow pushing source  %s\n"
				, fn->getName());
			CreateNodeFn createfn = fn->getCreateFn();
			EventBasePtr ev = (*createfn)(EventBase::no_event,NULL,fn);
			if(event::__acquire_guards(ev)) {
				events_vec.push_back(ev);
			}
		}
	}
	for(size_t i = 0; i < events_backend.size(); ++i) {
		events_vec.push_back(events_backend[i]);
	}
}

} // namespace event
} // namespace oflux
