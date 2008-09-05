#ifndef _OFLUX_EVENT_H
#define _OFLUX_EVENT_H

/**
 * @file OFluxEvent.h
 * @author Mark Pichora
 *  This is the header file for the Event template(s).
 *  The class is broken into three layers (ascending as you 
 *  know more about the type(s) involved (input and output)
 */

#include <boost/shared_ptr.hpp>
#include "OFlux.h"
#include "OFluxAtomicHolder.h"
//#include <vector>

namespace oflux {

//forward decl
template<typename G,typename H> H * convert(G *);
template<typename G,typename H> const H * const_convert(const G *);

class FlowNode;

/**
 * @class EventBase
 * @brief the event base class which covers the low level common operations
 * Events hold a reference to the previous event to hold onto the memory.
 * This keeps it from being reclaimed to the heap before the current event
 * is done executing.
 */
class EventBase : public FlowNodeCounterIncrementer {
public:
	static boost::shared_ptr<EventBase> no_event;

	EventBase(boost::shared_ptr<EventBase> predecessor,FlowNode *flow_node)
		: FlowNodeCounterIncrementer(flow_node)
		, _predecessor(predecessor)
		, _error_code(0)
		//, _flow_node(flow_node)
		{
			LinkedListRemovable<FlowGuardReference> & guards = 
				flow_node->guards();
			Removable<FlowGuardReference> * n = guards.first();
			while(n != NULL) {
				_atomics.add(n->content());
				n = n->next();
			}
		}
	virtual ~EventBase() {}
	virtual void * output_type() = 0;
	void release() { _predecessor = no_event; }
	virtual int execute() = 0;
	Atomic * acquire(AtomicsHolder & given_atomics)
		{
			int res = ( _predecessor == NULL
				? -1
				: _atomics.acquire(given_atomics,
					_predecessor->output_type()));
			return (res == -1 ? NULL : _atomics.get(res)->atomic());
		}
	void release(EventList & released_events)
		{ _atomics.release(released_events); }
	inline void error_code(int ec) { _error_code = ec; }
	inline int error_code() { return _error_code; }
	//inline FlowNode * flow_node() { return _flow_node; }
	inline AtomicsHolder & atomics() { return _atomics; }
	void log_snapshot();
protected:
	inline EventBase * get_predecessor()
		{ return _predecessor.get(); }
private:
	boost::shared_ptr<EventBase> _predecessor;
protected:
	int           _error_code;
	//FlowNode *    _flow_node;
	AtomicsHolder _atomics;
};

/**
 * @class EventBase2
 * @brief template which elaborates the basic input and output memory structures
 * This refinement of EventBase actually holds the memory used to 
 * output an event result.
 */
template<typename IT, typename OT, typename AM>
class EventBase2 : public EventBase {
public:
	EventBase2(boost::shared_ptr<EventBase> predecessor, FlowNode * flow_node)
		: EventBase(predecessor,flow_node)
		{
		}
	virtual void * output_type() { return &_ot; }
protected:
	inline const IT * pr_input_type() 
		{ EventBase * pred = get_predecessor();
		  return static_cast<IT*> (pred ? pred->output_type() : &theE); }
	inline OT * pr_output_type() { return &_ot; }
	inline AM * atomics_argument() { return &_am; }
private:
	OT            _ot;
protected:
	AM            _am;
};

/**
 * @class Event
 * @brief the final Event template containing the execution function
 * The execution function is specified as a template parameter.  
 * This highest level of the template hierarchy know the concrete data
 * structures manipulated as input and output.
 */
template<typename IT,typename OT,typename IM,typename OM, typename AM, int (*node_func)(const IM *, OM*, AM *)>
class Event : public EventBase2<IT,OT,AM> {
public:
	Event(boost::shared_ptr<EventBase> predecessor,FlowNode * flow_node)
		: EventBase2<IT,OT,AM>(predecessor,flow_node)
		{}
	virtual int execute()
		{ 
		  EventBase2<IT,OT,AM>::atomics_argument()->fill(&(this->atomics()));
		  int res = (*node_func)(
			const_convert<IT,IM>(EventBase2<IT,OT,AM>::pr_input_type()),
			convert<OT,OM>(EventBase2<IT,OT,AM>::pr_output_type()),
			EventBase2<IT,OT,AM>::atomics_argument()
			); 
		  if (!res) EventBase::release();
		  return res;
		}
};

/**
 * @class ErrorEvent
 * @brief A version of Event for error handler events
 * This kind of event has a C function associated with it that takes 
 * the error code as the final argument in its parameter list
 */
template<typename IT,typename OT,typename IM,typename OM, typename AM, int (*node_func)(const IM *, OM*, AM*, int)>
class ErrorEvent : public EventBase2<IT,OT,AM> {
public:
	ErrorEvent(boost::shared_ptr<EventBase> predecessor,FlowNode * flow_node)
		: EventBase2<IT,OT,AM>(predecessor,flow_node)
		{}
	virtual int execute()
		{ 
		  EventBase2<IT,OT,AM>::atomics_argument()->fill(&(this->atomics()));
		  int res = (*node_func)(
			const_convert<IT,IM>(EventBase2<IT,OT,AM>::pr_input_type()),
			convert<OT,OM>(EventBase2<IT,OT,AM>::pr_output_type()), 
			EventBase2<IT,OT,AM>::atomics_argument(),
			EventBase::error_code()); 
		  if (!res) EventBase::release();
		  return res;
		}
};

/**
 * @brief create an Event from a FlowNode
 * (this is a factory function)
 * @param pred_node_ptr  pointer to predecessor event
 * @param fn  flow node
 *
 * @return smart pointer to the new event (heap allocated)
 **/
template<typename IT,typename OT,typename IM,typename OM, typename AM, int (*node_func)(const IM *, OM*, AM*)>
boost::shared_ptr<EventBase> create(boost::shared_ptr<EventBase> pred_node_ptr,FlowNode *fn)
{
	return boost::shared_ptr<EventBase>(new Event<IT,OT,IM,OM,AM,node_func>(pred_node_ptr,fn));
}

/**
 * @brief create an Event from a source FlowNode
 * (this is a factory function)
 * @param fn  the source flow node
 *
 * @return  smart pointer managed new source event (heap allocated)
 **/
template<typename IT,typename OT,typename IM,typename OM, typename AM, int (*node_func)(const IM *, OM*, AM*)>
boost::shared_ptr<EventBase> create_source(FlowNode *fn)
{
	return boost::shared_ptr<EventBase>(new Event<IT,OT,IM,OM,AM,node_func>(EventBase::no_event, fn));
}

/**
 * @brief create and ErrorEvent from a FlowNode
 * (this is a factory function)
 * @param pred_node_ptr  predecessor event
 * @param fn  flow node
 *
 * @return smart pointer to the new error event (heap allocated)
 **/
template<typename IT,typename OT,typename IM,typename OM,typename AM, int (*node_func)(const IM *, OM*, AM*, int)>
boost::shared_ptr<EventBase> create_error(boost::shared_ptr<EventBase> pred_node_ptr, FlowNode * fn)
{
	return boost::shared_ptr<EventBase>(new ErrorEvent<IT,OT,IM,OM,AM,node_func>(pred_node_ptr,fn));
}


}; // namespace

#endif // _OFLUX_EVENT_H
