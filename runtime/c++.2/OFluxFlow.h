#ifndef _OFLUX_FLOW
#define _OFLUX_FLOW

/**
 * @file OFluxFlow.h
 * @author Mark Pichora
 * Flow data structure which encapsulates the program data flow.
 * This only changes if we have the server load a new flow graph.
 */

#include "OFlux.h"
#include "OFluxCondition.h"
#include "OFluxAtomic.h"
#include "OFluxProfiling.h"
#include "OFluxLinkedList.h"
#include <cassert>
#include <vector>
#include <map>
#include <string>
#include "boost/shared_ptr.hpp"

namespace oflux {



/**
 * @class FlowFunctionMaps
 * @brief holds the static maps generated by the flux compiler
 * The static map lookup algorithms are linear (sub-optimal), but this
 * does not matter much.
 */
class FlowFunctionMaps { // data that is compiled in
public:
	FlowFunctionMaps(ConditionalMap cond_map[],
			CreateMap create_map[],
			GuardTransMap guard_map[],
			AtomicMapMap atom_map[]);

	/**
	 * @brief lookup a create factory function from a table given the event name
	 * @param n name of the node  
	 * @return a function pointer usable to create a new event (smart pointered)
	 **/
	CreateNodeFn lookup_node(const char * n);

	/**
	 * @brief lookup a conditional function usable on a particular input
	 * @param n  name of the conditional
	 * @param argno  the number of the field in the object
	 * @param unionnumber  OFluxUnionX number
	 * @return 
	 **/
	ConditionFn lookup_conditional(const char * n, int argno, int unionnumber);
	/**
	 * @brief lookup a guard translator function
	 * @remark these functions can translate an input structure to a guard key structure
	 * @param guardname name of the guard
	 * @param union_number union number (indicates the input structure)
	 * @param argnos a vector of unique argument numbers used to pick off the fields
	 * @return the compiled function (pointer to it)
	 */
	GuardTransFn lookup_guard_translator(const char * guardname,
		int union_number, std::vector<int> & argnos);

	/**
	 * @brief lookup the atomic map object for the given guard 
	 * @param guardname the guard name
	 * @return the atomic map object
	 */
	AtomicMapAbstract * lookup_atomic_map(const char * guardname);
private:
	ConditionalMap * _cond_map;
	CreateMap *      _create_map;
	GuardTransMap *  _guard_trans_map;
	AtomicMapMap *   _atom_map_map;
};

/**
 * @class FlowCondition
 * @brief holder of a conditional (used to determine event flow)
 */
class FlowCondition {
public:
	FlowCondition(ConditionFn condfn, bool is_negated);
	inline bool satisfied(const void * a) 
		{ return ((*_condfn)(a) ? !_is_negated : _is_negated); }
private:
	ConditionFn _condfn;
	bool        _is_negated;
};

class Atomic;

/**
 * @class FlowGuard
 * @brief holder of a guared (used to implement atomic constraints)
 */
class FlowGuard {
public:
	FlowGuard(AtomicMapAbstract * amap, const char * n, int magic)
		: _amap(amap)
		, _name(n)
		, _magic_number(magic)
		{}
	/**
	 * @brief acquire the atomic value if possible -- otherwise should wait
	 * @param av_returned output argument the Atomic for this key
	 * @param key value used to dereference the atomic map
	 * @returns key that was permanently allocated on the heap
	 */
	inline const void * get(Atomic * & av_returned,
			const void * key)
		{ 
			return _amap->get(av_returned,key);
		}

	/**
         * @brief a comparator function for keys (punts to underlying atomic map)
         * @return -1 if <, 0 if ==, +1 if >
         */
	inline int compare_keys(const void *k1, const void * k2) const 
		{ return _amap->compare(k1,k2); }
	/**
	 * @brief allocate a new key and return a void pointer to it
	 * @return the key pointer
	 */
	inline void * new_key() { return _amap->new_key(); }
	/**
	 * @brief delete the allocated key
	 * @param k the key pointer to reclaim
	 */
	inline void delete_key(void * k) { _amap->delete_key(k); }
	/**
	 * @brief return the name of the guard
	 * @return name of the guard
	 */
	inline const std::string & getName() { return _name; }
	/**
	 * @brief the magic number is the index of the guard declaration
	 * @return magic number 
	 */
	inline int magic_number() const { return _magic_number; }
private:
	AtomicMapAbstract * _amap;
	std::string	    _name;
	int                 _magic_number;
};

/**
 * @class FlowGuardReference
 * @brief This is the guard instance part of the flow
 */
class FlowGuardReference {
public:
	FlowGuardReference(FlowGuard * fg)
		: _guardfn(NULL)
		, _flow_guard(fg)
		, _local_key(fg->new_key())
		{}
	~FlowGuardReference()
		{
			if(_local_key) {
				_flow_guard->delete_key(_local_key);
			}
		}
	/**
	 * @brief set the guard translator function for this guard instance
	 */
	inline void setGuardFn(GuardTransFn guardfn) { _guardfn = guardfn; }
	/**
	 * @brief used to dereference the atomic map to get the atomic and persistent key
	 * @param a_out output atomic pointer
	 * @param node_in the node input structure (used to generate a key)
	 */
	inline const void * get(Atomic * & a_out,const void * node_in)
		{ 
			(*_guardfn)(_local_key, node_in);
			return _flow_guard->get(a_out,_local_key);
		}
	/**
	 * @brief compare keys (pointers to void)
	 */
	inline int compare_keys(const void * k1, const void * k2)
		{
			return _flow_guard->compare_keys(k1,k2);
		}
	/**
	 * @brief return the magic number of the guard
	 */
	inline int magic_number() const { return _flow_guard->magic_number(); }
	/** 
	 * @brief return the name of the guard
	 */
	inline const std::string & getName() { return _flow_guard->getName(); }
private:
	GuardTransFn _guardfn;
	FlowGuard *  _flow_guard;
	void *       _local_key; // not reentrant here
};

class FlowNode;

/**
 * @class FlowCase
 * @brief given a set of conditions, each case has a node target
 * If ALL conditions are satisfied, then the node target is added to the 
 * event queue.
 */
class FlowCase {
public:
	FlowCase(FlowNode *targetnode=NULL);
	~FlowCase();
	void add(FlowCondition *fc);
	inline bool satisfied(const void * a)
	{
		bool res = true;
		Node<FlowCondition> * n = _conditions.first();
		while(n != NULL && res) {
			res = res && n->content()->satisfied(a);
			n = n->next();
		}
		return res;
	}
	inline FlowNode * targetNode() { return _targetnode; }
	inline void setTargetNode(FlowNode * fn) { _targetnode = fn; }
private:
	FlowNode *                   _targetnode;
	LinkedList<FlowCondition> _conditions;
};

/**
 * @class FlowSuccessor
 * @brief a container for a set of flow cases which are operated in order 
 * The first containing flow case to have all of its conditions met,
 * is used.
 */
class FlowSuccessor {
public:
	FlowSuccessor();
	~FlowSuccessor();
	void add(FlowCase * fc);
	inline FlowNode * get_successor(const void * a) 
	{
		FlowNode * res = NULL;
		Node<FlowCase> * n = _cases.first();
		while(n != NULL && res == NULL) {
			FlowCase * flowcase = n->content();
			if(flowcase->satisfied(a)) {
				res = flowcase->targetNode();
			}
			n = n->next();
		}
		assert(res);
		return res;
	}
private:
	LinkedListRemovable<FlowCase> _cases;
};

/**
 * @class FlowSuccessorList
 * @brief A list of concurrent flow successors (all are evaluated)
 * This will result (in general) with multiple successor node targets.
 */
class FlowSuccessorList {
public:
	FlowSuccessorList();
	~FlowSuccessorList();
	void add(FlowSuccessor * fs);
	inline void get_successors(LinkedList<FlowNode> & successor_nodes, const void * a)
	{
		Node<FlowSuccessor> * n = _successorlist.first();
		while(n != NULL) {
			FlowNode * s = n->content()->get_successor(a);
			successor_nodes.insert_front(s);
			n = n->next();
		}
	}
private:
	LinkedList<FlowSuccessor> _successorlist;
};


/**
 * @class FlowNode
 * @brief holds the programmatic information about a node (err handler, succ list)
 * Holds the name, error handler, event factory function, 
 * and flow successor list of a flow node.  
 * Also maintains the info that the node is a source (if it is).
 */
class FlowNode {
public:
	friend class FlowNodeCounterIncrementer;
	FlowNode(const char * name,
		CreateNodeFn createfn,
		bool is_error_handler,
		bool is_source,
		bool is_detached);
	~FlowNode();
	void setErrorHandler(FlowNode *fn);
	FlowSuccessorList & successor_list() { return _successor_list; }
	inline const char * getName() { return &(_name[0]); }
	inline bool getIsSource() { return _is_source; }
	inline bool getIsErrorHandler() { return _is_error_handler; }
	inline bool getIsDetached() { return _is_detached; }
	inline CreateNodeFn & getCreateFn() { return _createfn; }
	inline void get_successors(LinkedList<FlowNode> & successor_nodes, 
			const void * a,
			int return_code)
	{
		if(return_code != 0) {
			if(_error_handler != NULL) {
				successor_nodes.insert_front(_error_handler);
			} else if(_is_source) {
				successor_nodes.insert_front(this);
			}
		} else {
			_successor_list.get_successors(successor_nodes,a);
		}
	}
	inline LinkedListRemovable<FlowGuardReference> & guards() 
		{ return _guard_refs; }
	void addGuard(FlowGuardReference * fgr) { _guard_refs.insert_back(fgr); }
	void log_snapshot();
#ifdef PROFILING
	inline TimerStats * real_timer_stats() { return &_real_timer_stats; }
	inline TimerStats * oflux_timer_stats() { return &_oflux_timer_stats; }
#endif
	inline int instances() { return _instances; }
	inline void turn_off_source() { _is_source = false; }
protected:
	int                               _instances; // count active events
	int                               _executions; // count active events
private:
	std::string                       _name;
	CreateNodeFn                      _createfn;
	bool                              _is_error_handler;
	bool                              _is_source;
	bool                              _is_detached;
	FlowSuccessorList                 _successor_list;
	FlowNode *                        _error_handler;
	LinkedListRemovable<FlowGuardReference> _guard_refs;
#ifdef PROFILING
	TimerStats                        _real_timer_stats;
	TimerStats                        _oflux_timer_stats;
#endif
};

class FlowNodeCounterIncrementer {
public:
	FlowNodeCounterIncrementer(FlowNode * flow_node)
		: _flow_node(flow_node)
		{ 
			_flow_node->_instances++; 
			_flow_node->_executions++; 
		}
	~FlowNodeCounterIncrementer()
		{ _flow_node->_instances--; }
	inline FlowNode * flow_node() { return _flow_node; }
protected:
	FlowNode * _flow_node;
};

/**
 * @class Flow
 * @brief a flow is a collection of flow nodes (connected internally)
 */
class Flow {
public:
	Flow();
	~Flow();

	/**
	 * @brief get a vector of sources in the flow
	 * @return a vector of sources
	 **/
	LinkedListRemovable<FlowNode> & sources();

	/**
	 * @brief add a flow node
	 * @param fn FlowNode to add to the flow
	 **/
	void add(FlowNode *fn)
	{
		std::pair<std::string,FlowNode *> pr(fn->getName(),fn);
		_nodes.insert(pr);
		if(fn->getIsSource()) {
			_sources.insert_front(fn);
		}
	}

	/**
	 * @brief obtain a flow node by name
	 * @param name given a name of a flow node
	 * @return the flow node of that name (NULL for not found)
	 **/
	FlowNode * get(const std::string & name)
	{
		std::map<std::string, FlowNode *>::iterator itr = 
			_nodes.find(name);
		return (itr == _nodes.end() ? NULL : (*itr).second);
	}

	FlowGuard * getGuard(const std::string & name)
	{
		std::map<std::string, FlowGuard *>::iterator itr = 
			_guards.find(name);
		return (itr == _guards.end() ? NULL : (*itr).second);
	}

	void addGuard(FlowGuard *fg) 
	{
		_guards[fg->getName()] = fg;
	}
	void log_snapshot();
	void turn_off_sources()
	{
		Removable<FlowNode> * n = _sources.first();
		while(n != NULL) {
			n->content()->turn_off_source();
			Removable<FlowNode> * nrem = n;
			n = n->next();
			_sources.remove(nrem);
		}
	}
	bool has_instances() 
	{
		bool res = false;
		std::map<std::string, FlowNode *>::iterator mitr = 
			_nodes.begin();
		while(mitr != _nodes.end() && !res) {
			res = res || ((*mitr).second->instances() > 0);
			mitr++;
		}
		return res;
	}
private:
	std::map<std::string, FlowNode *>  _nodes;
	LinkedListRemovable<FlowNode> _sources;
	std::map<std::string, FlowGuard *> _guards;
};

}; // namespace


#endif // _OFLUX_FLOW

