#include "OFluxFlow.h"
#include "OFluxLogging.h"
#include <stdlib.h>
#include <cassert>

/**
 * @filename OFluxFLow.cpp
 * @author Mark Pichora
 * Flow implementation
 */

namespace oflux {

FlowFunctionMaps::FlowFunctionMaps(ConditionalMap cond_map[],
		                        CreateMap create_map[],
					GuardTransMap guard_map[],
					AtomicMapMap atom_map_map[])
	: _cond_map(cond_map)
	, _create_map(create_map)
	, _guard_trans_map(guard_map)
	, _atom_map_map(atom_map_map)
{}

// linear searches are fast enough -- its just for XML converstion...

CreateNodeFn FlowFunctionMaps::lookup_node(const char * n)
{
	CreateNodeFn res = NULL;
	CreateMap * cm = _create_map;
	while(cm->name) {
		if(strcmp(cm->name,n) == 0) {
			res = cm->createfn;
			break;
		}
		cm++;
	}
	return res;
}

ConditionFn FlowFunctionMaps::lookup_conditional(const char * n, int argno, int unionnumber)
{
	ConditionFn res = NULL;
	ConditionalMap * cm = _cond_map;
	while(cm->name) {
		if(strcmp(cm->name, n) == 0
				&& argno == cm->argno
				&& unionnumber == cm->unionnumber) {
			res = cm->condfn;
			break;
		}
		cm++;
	}
	return res;
}

GuardTransFn FlowFunctionMaps::lookup_guard_translator(const char * guardname,
                int union_number, std::vector<int> & argnos)
{
	GuardTransFn res = NULL;
	GuardTransMap * ptr = _guard_trans_map;
	while(ptr->guardname != NULL) {
		if(strcmp(guardname, ptr->guardname) == 0
				&& union_number == ptr->union_number) {
			bool allargs = true;
			int i = 0;
			for(i = 0; i < (int)argnos.size() && allargs; i++) {
				if(ptr->argnos[i] == 0) {
					break;
				}
				allargs = (ptr->argnos[i] == argnos[i]);
			}
			allargs = allargs && ((int)argnos.size() == i);
			if(allargs) {
				res = ptr->guardtransfn;
				break;
			}
		}
		ptr++;
	}
	return res;
}

AtomicMapAbstract * FlowFunctionMaps::lookup_atomic_map(const char * guardname)
{
	AtomicMapAbstract * res = NULL;
	AtomicMapMap * ptr = _atom_map_map;
	while(ptr->guardname != NULL) {
		if(strcmp(guardname, ptr->guardname) == 0) {
			res = ptr->amap;
			break;
		}
		ptr++;
	}
	return res;
}

FlowCondition::FlowCondition(ConditionFn condfn, bool is_negated)
	: _condfn(condfn)
	, _is_negated(is_negated)
	{}

template<class C, class N>
void delete_linked_list_contents(LinkedList<C,N> & ll)
{
	N * n = ll.first();
	while(n != NULL) {
		delete n->content();
		n = n->next();
	}
}

template<class O>
void delete_vector_contents(std::vector<O *> & vo)
{
	for(int i = 0; i < (int) vo.size(); i++) {
		delete vo[i];
	}
}

template<typename K, typename O>
void delete_map_contents(std::map<K,O *> & mo)
{
	typename std::map<K,O *>::iterator mitr;
       	mitr = mo.begin();
	while(mitr != mo.end()) {
		delete (*mitr).second;
		mitr++;
	}
}

FlowCase::FlowCase(FlowNode *targetnode)
	: _targetnode(targetnode)
{
}

FlowCase::~FlowCase() 
{ 
	delete_linked_list_contents<FlowCondition, Node<FlowCondition> >(_conditions);
	//delete_vector_contents<FlowCondition>(_conditions); 
}

void FlowCase::add(FlowCondition *fc) 
{ 
	_conditions.insert_front(fc); 
}

FlowSuccessor::FlowSuccessor()
{
}

FlowSuccessor::~FlowSuccessor() 
{ 
	delete_linked_list_contents<FlowCase, Removable<FlowCase, Node<FlowCase> > >(_cases);
	//delete_vector_contents<FlowCase>(_cases); 
}

void FlowSuccessor::add(FlowCase * fc) 
{ 
	_cases.insert_back(fc); 
}

FlowSuccessorList::FlowSuccessorList()
{
}

FlowSuccessorList::~FlowSuccessorList() 
{ 
	delete_linked_list_contents<FlowSuccessor, Node<FlowSuccessor> >(_successorlist);
}

void FlowSuccessorList::add(FlowSuccessor * fs) 
{ 
	_successorlist.insert_front(fs); 
}

FlowNode::FlowNode(const char * name,
	CreateNodeFn createfn,
	bool is_error_handler,
	bool is_source,
	bool is_detached)
	: _instances(0)
	, _executions(0)
	, _name(name)
	, _createfn(createfn)
	, _is_error_handler(is_error_handler)
	, _is_source(is_source)
	, _is_detached(is_detached)
	, _error_handler(NULL)
{ 
}

FlowNode::~FlowNode()
{
	delete_linked_list_contents<FlowGuardReference, Removable<FlowGuardReference, Node<FlowGuardReference> > >(_guard_refs);
	//delete_vector_contents<FlowGuardReference>(_guard_refs);
}

void FlowNode::setErrorHandler(FlowNode *fn)
{
	_error_handler = fn;
}

void FlowNode::log_snapshot()
{
#ifdef PROFILING
	oflux_log_info("%s (%c%c%c) %d instances %d executions (time real:avg %lf max %lf oflux:avg %lf max %lf)\n", 
		_name.c_str(),
		(_is_source ? 's' : '-'),
		(_is_detached ? 'd' : '-'),
		(_is_error_handler ? 'e' : '-'),
		_instances,
		_executions,
		_real_timer_stats.avg_usec(),
		_real_timer_stats.max_usec(),
		_oflux_timer_stats.avg_usec(),
		_oflux_timer_stats.max_usec());	
#else
	oflux_log_info("%s (%c%c%c) %d instances %d executions\n",
		_name.c_str(),
		(_is_source ? 's' : '-'),
		(_is_detached ? 'd' : '-'),
		(_is_error_handler ? 'e' : '-'),
		_instances,
		_executions);
#endif
}

Flow::Flow()
{
}

Flow::~Flow() 
{ 
	delete_map_contents<std::string,FlowNode>(_nodes); 
	delete_map_contents<std::string,FlowGuard>(_guards); 
}

void Flow::log_snapshot()
{
	std::map<std::string, FlowNode *>::iterator mitr =
		_nodes.begin();
	while(mitr != _nodes.end()) {
		(*mitr).second->log_snapshot();
		mitr++;
	}
}

LinkedListRemovable<FlowNode> & Flow::sources()
{
	return _sources;
}

}; // namespace
