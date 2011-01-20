#include <string.h>
#include "OFluxFlow.h"
#include "OFluxLogging.h"
#include "OFluxLibrary.h"
#include <stdlib.h>
#include <algorithm>
#include <cassert>

/**
 * @author Mark Pichora
 * @brief Flow implementation
 *   A flow is the resident data structure that captures the flow of the 
 *   program (it is read from an XML input to the oflux runtime)
 */

namespace oflux {
namespace flow {

FunctionMaps::FunctionMaps(ConditionalMap cond_map[],
                ModularCreateMap create_map[],
                GuardTransMap guard_map[],
                AtomicMapMap atom_map_map[],
                IOConverterMap ioconverter_map[])
        : _cond_map(cond_map)
        , _create_map(create_map)
        , _guard_trans_map(guard_map)
        , _atom_map_map(atom_map_map)
        , _ioconverter_map(ioconverter_map)
{}

// linear searches are fast enough -- its just for XML converstion...

CreateNodeFn 
FunctionMaps::lookup_node_function(const char * n) const
{
        CreateNodeFn res = NULL;
        ModularCreateMap * mcm = _create_map;
        while(mcm->scope) {
                int sz = strlen(mcm->scope);
                if(strncmp(mcm->scope,n,sz) == 0) {
                        CreateMap * cm = mcm->map;
                        while(cm->name) {
                                if(strcmp(cm->name,n + sz) == 0) {
                                        res = cm->createfn;
                                        break;
                                }
                                cm++;
                        }
                        if(res) {
                                break;
                        }
                }
                mcm++;
        }
        return res;
}

ConditionFn 
FunctionMaps::lookup_conditional(
	  const char * n
	, int argno
	, int unionnumber) const
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

GuardTransFn 
FunctionMaps::lookup_guard_translator(
	  const char * guardname
	, int union_number
	, const char * hash
	, int wtype
	, bool late) const
{
        GuardTransFn res = NULL;
        GuardTransMap * ptr = _guard_trans_map;
        while(ptr->guardname != NULL) {
                if(strcmp(guardname, ptr->guardname) == 0
                                && union_number == ptr->union_number
                                && 0 == strcmp(hash,ptr->hash)
                                && wtype == ptr->wtype
                                && late == ptr->late
                                ) {
                        res = ptr->guardtransfn;
                        break;
                }
                ptr++;
        }
        return res;
}

AtomicMapAbstract * 
FunctionMaps::lookup_atomic_map(const char * guardname) const
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

FlatIOConversionFun 
FunctionMaps::lookup_io_conversion(
	  int from_unionnumber
	, int to_unionnumber) const
{
        FlatIOConversionFun res = NULL;
        IOConverterMap * ptr = _ioconverter_map;
        while(ptr->from > 0) {
                if(ptr->from == from_unionnumber && ptr->to == to_unionnumber) {
                        res = ptr->conversion_fun;
			oflux_log_info("found entry at %s:%d for "
				"IO conversion from %s to %s\n"
				, ptr->file
				, ptr->line
				, ptr->from_str_name
				, ptr->to_str_name);
                        break;
                }
                ptr++;
        }
        return res;
}

Condition::Condition(ConditionFn condfn, bool is_negated)
        : _condfn(condfn)
        , _is_negated(is_negated)
        {}

template<class O>
void 
delete_vector_contents(std::vector<O *> & vo)
{
        for(int i = 0; i < (int) vo.size(); i++) {
                delete vo[i];
        }
}

template<class O>
void 
delete_vector_contents_reversed(std::vector<O *> & vo)
{
        for(int i = ((int)vo.size())-1; i >= 0 ; --i) {
                delete vo[i];
        }
}

template<class O>
void 
delete_deque_contents(std::deque<O *> & mo)
{
        typename std::deque<O *>::iterator itr;
        itr = mo.begin();
        while(itr != mo.end()) {
                delete (*itr);
                itr++;
        }
}

template<typename K, typename O>
void 
delete_map_contents(std::map<K,O *> & mo)
{
        typename std::map<K,O *>::iterator mitr;
        mitr = mo.begin();
        while(mitr != mo.end()) {
                delete (*mitr).second;
                mitr++;
        }
}

std::string 
create_indention(int depth)
{
        std::string indention;
        for(int i = 0; i < depth; i++) {
                indention = indention + "  ";
        }
        if(depth) {
                indention = indention + "|_";
        }
        return indention;
}

void
Guard::drain()
{
        std::vector<boost::shared_ptr<EventBase> > rel_vector;
        boost::shared_ptr<AtomicMapWalker> walker(_amap->walker());
        const void * v = NULL;
        Atomic * a = NULL;
        while(walker->next(v,a)) {
                a->release(rel_vector);
        }
}

Case::Case(Node *targetnode, IOConverter *converter)
        : _targetnode(targetnode)
        , _io_converter(converter ? converter : &IOConverter::standard_converter)
{
}

IOConverter IOConverter::standard_converter(create_trivial_io_conversion<const void>);

Case::~Case() 
{ 
        delete_vector_contents<Condition>(_conditions); 
        if(_io_converter != NULL && _io_converter != &IOConverter::standard_converter) {
                delete _io_converter;
        }
}

void 
Case::add(Condition *fc) 
{ 
        _conditions.push_back(fc); 
}

void 
Case::pretty_print(int depth, std::set<std::string> * visited)
{
        _targetnode->pretty_print(depth,
                (_conditions.size() == 0 ? 'D' : 'C'), visited);
}

Successor::Successor(const char * name)
        : _name(name)
{
}

Successor::~Successor() 
{ 
        delete_deque_contents<Case>(_cases); 
}

void 
Successor::add(Case * fc, bool front) 
{ 
        if(front) {
                _cases.push_front(fc);
        } else {
                _cases.push_back(fc);
        }
}

void
Successor::remove(Case * fc)
{
	std::deque<Case *>::iterator itr = _cases.begin();
	bool fd = false;
	while(itr != _cases.end()) {
		if(fc == (*itr)) {
			_cases.erase(itr);
			delete fc;
			break;
		}
		++itr;
	}
	assert(fd && "Successor::remove failed to find");
}

void 
Successor::pretty_print(int depth, std::set<std::string> * visited)
{
        // do not print successor for now
        std::deque<Case *>::iterator itr = _cases.begin();
        while(itr != _cases.end()) {
                (*itr)->pretty_print(depth+1,visited);
                itr++;
        }
        oflux_log_info("%s..%s\n", create_indention(depth+1).c_str(),_name.c_str());
}

SuccessorList::SuccessorList()
{
}

SuccessorList::~SuccessorList() 
{ 
        delete_map_contents<std::string, Successor>(_successorlist); 
}

void 
SuccessorList::add(Successor * fs) 
{ 
        std::pair<std::string,Successor *> pr(fs->getName(),fs);
        _successorlist.insert(pr); 
}

void 
SuccessorList::remove(Successor * fs) 
{ 
        std::map<std::string, Successor *>::iterator itr = 
		_successorlist.find(fs->getName());
        assert(fs == (*itr).second);
	_successorlist.erase(itr);
	delete fs;
}

void 
SuccessorList::pretty_print(int depth, std::set<std::string> * visited)
{
        std::map<std::string, Successor *>::iterator itr = _successorlist.begin();
        while(itr != _successorlist.end()) { 
                itr->second->pretty_print(depth+1,visited);
                ++itr;
        }
        oflux_log_info("%s.\n", create_indention(depth+1).c_str());
}

Node::Node(const char * name,
                CreateNodeFn createfn,
                bool is_error_handler,
                bool is_source,
                bool is_detached,
                int input_unionnumber,
                int output_unionnumber)
        : _instances(0)
        , _executions(0)
        , _name(name)
        , _createfn(createfn)
        , _is_error_handler(is_error_handler)
        , _is_source(is_source)
        , _is_detached(is_detached)
        , _this_case(this,NULL)
        , _input_unionnumber(input_unionnumber)
        , _output_unionnumber(output_unionnumber)
        , _is_guards_completely_sorted(false)
{ 
}

Node::~Node()
{
        delete_vector_contents<GuardReference>(_guard_refs);
}

struct CompareGuardsInNode {
        // our version of a < comparator
        bool operator()(GuardReference * gr1, GuardReference * gr2)
        {
                return gr1->magic_number() < gr2->magic_number();
        }
};
        
void
Node::sortGuards()
{
        CompareGuardsInNode acompare;
        // sort all guard reference ascending by magic number
        // it does not matter to much how slow std::sort is.
        std::sort(_guard_refs.begin(), _guard_refs.end(),acompare);
        // determine once and for all if the sort has magic number dupes
        _is_guards_completely_sorted = true;
        int last_magic_number = -1;
        for(size_t i = 0; i < _guard_refs.size(); ++i) {
                int mn = _guard_refs[i]->magic_number();
                if(last_magic_number >= mn) {
                        _is_guards_completely_sorted = false;
                        break;
                }
                last_magic_number = mn;
        }
}       

void 
Node::setErrorHandler(Node *fn)
{
        _error_handler_case.setTargetNode(fn);
}

void 
Node::log_snapshot()
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

void 
Node::pretty_print(int depth, char context, std::set<std::string> * visited)
{
        oflux_log_info("%s%c %s\n", create_indention(depth).c_str(),context, _name.c_str());
        if (visited->find(_name)==visited->end()) {
                visited->insert(_name);
                // if a source node is some node's successor(depth>0 && is_source), do not re-print its sucessor list
                if((depth == 0 ) || (!_is_source)) {
                        _successor_list.pretty_print(depth+1,visited);
                }
        }
}

Flow::~Flow() 
{ 
        delete_map_contents<std::string,Node>(_nodes); 
        delete_map_contents<std::string,Guard>(_guards); 
        if(!_gaveup_libraries) {
		delete_vector_contents_reversed<Library>(_libraries);
	}
}

void
Flow::log_snapshot_guard(const char * guardname)
{
	std::map<std::string, Guard *>::iterator itr = _guards.find(guardname);
	(*itr).second->log_snapshot();
}

Library * 
Flow::getPrevLibrary(const char * name)
{
        Library * prevlib = NULL;
        for(int i = 0; i <(int)_prev_libraries.size(); i++) {
                if(_prev_libraries[i]->getName() == name) {
                        prevlib = _prev_libraries[i];
                        break;
                }
        }
	return prevlib;
}

bool 
Flow::haveLibrary(const char * name)
{
        bool fd = false;
        for(int i = 0; i <(int)_libraries.size(); i++) {
                if(_libraries[i]->getName() == name) {
                        fd = true;
                        break;
                }
        }
        oflux_log_debug("Flow::haveLibrary() %s, returns %s\n"
                , name
                , fd ? "true" : "false");
        return fd;
}

void 
Flow::log_snapshot()
{
        std::map<std::string, Node *>::iterator mitr = _nodes.begin();
        while(mitr != _nodes.end()) {
                (*mitr).second->log_snapshot();
                mitr++;
        }
}

void 
Flow::pretty_print()
{
        for(int i=0; i < (int) _sources.size(); i++) {
                std::set<std::string> visited;
                _sources[i]->pretty_print(0,'S',&visited);
        }
        /*
        std::map<std::string, FlowNode *>::iterator mitr = _nodes.begin();
        while(mitr != _nodes.end()) {
                (*mitr).second->pretty_print(0);
                mitr++;
        }
        */
}

std::vector<Node *> & 
Flow::sources()
{
        return _sources;
}


void 
Flow::assignMagicNumbers() 
{ 
        std::map<std::string,Guard *>::iterator itr = _guards.begin();
        while(itr != _guards.end()) {
                _magic_sorter.addPoint((*itr).second->getName().c_str());
                itr++;
        }
        _magic_sorter.numberAll(); 
        itr = _guards.begin();
        while(itr != _guards.end()) {
                oflux_log_debug("flow assigned guard %s magic no %d\n", (*itr).second->getName().c_str(), (*itr).second->magic_number());
                itr++;
        }
        std::map<std::string, Node *>::iterator nitr = _nodes.begin();
        while(nitr != _nodes.end()) {
                (*nitr).second->sortGuards();
                nitr++;
        }
}


MagicNumberable * GuardMagicSorter::getMagicNumberable(const char * c)
{ 
        std::string cs(c); 
        return _flow->getGuard(cs); 
}

Flow::Flow(const Flow & existing_flow)
: _magic_sorter(this)
{
	for(size_t i = 0; i < existing_flow._libraries.size(); ++i) {
		_prev_libraries.push_back(existing_flow._libraries[i]);
	}
	existing_flow._gaveup_libraries = true;
}

void
Flow::getLibraryNames(std::vector<std::string> & result)
{
        for(size_t i = 0; i < _libraries.size(); i++ ) {
                if(_libraries[i]) {
                        result.push_back(_libraries[i]->getName());
                }
        }
}

void 
Flow::addLibrary(Library * lib, void *init_plugin_params)
{
        _libraries.push_back(lib);
        oflux_log_debug("Flow::addLibrary() %s\n"
                , lib->getName().c_str());
	// init plugin
	if(!getPrevLibrary(lib->getName().c_str())) {
		lib->init(init_plugin_params);
	}
}

void 
Flow::drainGuardsOfEvents()
{
        std::map<std::string,Guard *>::iterator gitr = _guards.begin();
        while(gitr != _guards.end()) {
                (*gitr).second->drain();
                gitr++;
        }
}

NodeCounterIncrementer::~NodeCounterIncrementer()
{
    _flow_node->_instances--;
}

} // namespace flow
} // namespace oflux
