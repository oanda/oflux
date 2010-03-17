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
#include "OFluxIOConversion.h"
#include "OFluxOrderable.h"
#include "OFluxLogging.h"
#include <cassert>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <string>
#include "boost/shared_ptr.hpp"

namespace oflux {

class Atomic;

namespace flow {

/**
 * @class FunctionMaps
 * @brief holds the static maps generated by the flux compiler
 * The static map lookup algorithms are linear (sub-optimal), but this
 * does not matter much.
 */
class FunctionMaps { // data that is compiled in
public:
        FunctionMaps(ConditionalMap cond_map[],
                        ModularCreateMap create_map[],
                        GuardTransMap guard_map[],
                        AtomicMapMap atom_map[],
                        IOConverterMap ioconverter_map[]);

        /**
         * @brief lookup a create factory function from a table given the event name
         * @param n name of the node  
         * @return a function pointer usable to create a new event (smart pointered)
         **/
        CreateNodeFn lookup_node_function(const char * n) const;

        /**
         * @brief lookup a conditional function usable on a particular input
         * @param n  name of the conditional
         * @param argno  the number of the field in the object
         * @param unionnumber  OFluxUnionX number
         * @return 
         **/
        ConditionFn lookup_conditional(const char * n, int argno, int unionnumber) const;
	/**
	 * @brief lookup a guard translator function
	 * @remark these functions can translate an input structure to a guard key structure
	 * @param guardname name of the guard
	 * @param union_number union number (indicates the input structure)
	 * @param hash is the hash result of on the expression (compiler's)
	 * @param wtype is the enumerated type for the guard(Read/Exclusive/...)
	 * @param late is true when this lookup is done for a late guard acquisition
	 * @return the compiled function (pointer to it)
	 */
        GuardTransFn lookup_guard_translator(
		  const char * guardname
                , int union_number
                , const char * hash
                , int wtype
		, bool late) const;

        /**
         * @brief lookup the atomic map object for the given guard 
         * @param guardname the guard name
         * @return the atomic map object
         */
        AtomicMapAbstract * lookup_atomic_map(const char * guardname) const;

        /**
         * @brief lookup the conversion from one type union to another
         * @return a generic function that can create an object that does the job
         */
        FlatIOConversionFun lookup_io_conversion(int from_unionnumber, int to_unionnumber) const;
private:
        ConditionalMap *   _cond_map;
        ModularCreateMap * _create_map;
        GuardTransMap *    _guard_trans_map;
        AtomicMapMap *     _atom_map_map;
        IOConverterMap *   _ioconverter_map;
};

/**
 * @class Condition
 * @brief holder of a conditional (used to determine event flow)
 */
class Condition {
public:
        Condition(ConditionFn condfn, bool is_negated);
        inline bool satisfied(const void * a) 
        { return ((*_condfn)(a) ? !_is_negated : _is_negated); }
private:
        ConditionFn _condfn;
        bool        _is_negated;
};


/**
 * @class Guard
 * @brief holder of a guared (used to implement atomic constraints)
 */
class Guard : public MagicNumberable {
public:
        Guard(AtomicMapAbstract * amap, const char * n)
                : _amap(amap)
                , _name(n)
                {}
        /**
         * @brief acquire the atomic value if possible -- otherwise should wait
         * @param av_returned output argument the Atomic for this key
         * @param key value used to dereference the atomic map
         * @returns key that was permanently allocated on the heap
         */
        inline const void * 
	get(      Atomic * & av_returned
		, const void * key)
        { return _amap->get(av_returned,key); }

        /**
         * @brief a comparator function for keys (punts to underlying atomic map)
         * @return -1 if <, 0 if ==, +1 if >
         */
        inline int 
	compare_keys(const void *k1, const void * k2) const 
        { return _amap->compare(k1,k2); }
        /**
         * @brief allocate a new key and return a void pointer to it
         * @return the key pointer
         */
        inline void * 
	new_key() const { return _amap->new_key(); }
        /**
         * @brief delete the allocated key
         * @param k the key pointer to reclaim
         */
        inline void 
	delete_key(void * k) const { _amap->delete_key(k); }
        /**
         * @brief return the name of the guard
         * @return name of the guard
         */
        inline const std::string & 
	getName() { return _name; }
        /**
         * @brief release all events from this guard's queues
         */
        void drain();
	void log_snapshot() { if(_amap) _amap->log_snapshot(_name.c_str()); }
private:
        AtomicMapAbstract * _amap;
        std::string         _name;
};

/**
 * @class GuardLocalKey
 * @brief hold a local guard key and dispose of it when done
 */
class GuardLocalKey {
public:
	GuardLocalKey(Guard *g)
		: _local_key(g->new_key())
		, _guard(g)
	{}
	~GuardLocalKey()
	{ _guard->delete_key(_local_key); }
	void * get() { return _local_key; }
private:
	void * _local_key;
	Guard * _guard;
};


/**
 * @class GuardReference
 * @brief This is the guard instance part of the flow
 */
class GuardReference {
public:
        GuardReference(Guard * fg, int wtype, bool late)
                : _guardfn(NULL)
                , _flow_guard(fg)
                , _wtype(wtype)
		, _late(late)
                , _lexical_index(-1)
	{}
        ~GuardReference()
        {
        }
        /**
         * @brief set the guard translator function for this guard instance
         */
        inline void 
	setGuardFn(GuardTransFn guardfn) { _guardfn = guardfn; }
        /**
         * @brief used to dereference the atomic map to get the atomic 
         *        and persistent key
         * @param a_out output atomic pointer
         * @param node_in the node input structure (used to generate a key)
	 * @param ah the atomic holder for this acquisition
         */
        inline const void * 
	get(      Atomic * & a_out
		, const void * node_in
		, AtomicsHolderAbstract * ah)
	{ 
		bool ok = true;
		GuardLocalKey local_key(_flow_guard);
		try {
			if(_guardfn) {
				ok = (*_guardfn)(local_key.get(), node_in, ah);
			}
		} catch (AHAException & ahae) {
			ok = false;
			oflux_log_warn("oflux::flow::GuardReference::get() threw an AHAException -- so an earlier guard was not aquired %s, guard arg %d\n", ahae.func(), ahae.index());
		}
		if(!ok) {
			a_out = NULL;
			return NULL;
		}
		return _flow_guard->get(a_out,local_key.get());
        }
        /**
         * @brief compare keys (pointers to void)
         */
        inline int 
	compare_keys(const void * k1, const void * k2)
        {
                return _flow_guard->compare_keys(k1,k2);
        }
        /**
         * @brief return the magic number of the guard
         */
        inline int 
	magic_number() const { return _flow_guard->magic_number(); }
        /** 
         * @brief return the name of the guard
         */
        inline const std::string & getName() { return _flow_guard->getName(); }

        inline int wtype() const { return _wtype; }
        inline void setLexicalIndex(int i) { _lexical_index = i; }
        inline int getLexicalIndex() const { return _lexical_index; }
	inline bool late() const { return _late; }
private:
        GuardTransFn _guardfn;
        Guard *      _flow_guard;
        int          _wtype;
	bool         _late;
        int          _lexical_index;
};

class Node;

class IOConverter {
public:
        static IOConverter standard_converter; 

        IOConverter(FlatIOConversionFun conversionfun)
                        : _io_conversion(conversionfun)
        {}
        inline const void * convert(const void * out) const 
        {
                return _io_conversion ? (*_io_conversion)(out) : NULL;
        }
private:
        FlatIOConversionFun _io_conversion;
};

/**
 * @class Case
 * @brief given a set of conditions, each case has a node target
 * If ALL conditions are satisfied, then the node target is added to the 
 * event queue.
 */
class Case {
public:
        Case(Node *targetnode=NULL, IOConverter * converter=NULL);
        ~Case();
        void add(Condition *fc);
        inline bool satisfied(const void * a)
        {
                bool res = true;
                for(int i = 0; i <(int)_conditions.size() && res; i++) {
                        res = res && _conditions[i]->satisfied(a);
                }
                return res;
        }
        inline Node * targetNode() { return _targetnode; }
        inline IOConverter * ioConverter() { return _io_converter; }
        inline void setTargetNode(Node * fn) { _targetnode = fn; }
        inline void setIOConverter(IOConverter * fioc) { _io_converter = fioc; }
        void pretty_print(int depth, std::set<std::string> * visited);
        inline bool isDefault() { return _conditions.size() == 0; }
private:
        Node *                   _targetnode;
        IOConverter *            _io_converter;
        std::vector<Condition *> _conditions;
};

/**
 * @class Successor
 * @brief a container for a set of flow cases which are operated in order 
 * The first containing flow case to have all of its conditions met,
 * is used.
 */
class Successor {
public:
        Successor(const char * name);
        ~Successor();
        inline const std::string & getName() { return _name; }
        void add(Case * fc, bool front=false);
	void remove(Case * fc);
        inline Case * get_successor(const void * a) 
        {
                Case * res = NULL;
                std::deque<Case *>::iterator itr = _cases.begin();
                while(itr != _cases.end() && res == NULL) {
                        Case * flowcase = *itr;
                        if(flowcase->satisfied(a)) {
                                res = flowcase;
                        }
                        itr++;
                }
                return res;
        }
        void pretty_print(int depth, std::set<std::string> * visited);
private:
        std::string        _name;
        std::deque<Case *> _cases;
};

/**
 * @class SuccessorList
 * @brief A list of concurrent flow successors (all are evaluated)
 * This will result (in general) with multiple successor node targets.
 */
class SuccessorList {
public:
        SuccessorList();
        ~SuccessorList();
        bool empty() { return _successorlist.empty(); }
        void add(Successor * fs);
        void remove(Successor * fs);
        Successor * get_successor(const char * name)
        {
                std::map<std::string, Successor *>::iterator itr = _successorlist.find(name);
                return (itr ==  _successorlist.end() ? NULL : (*itr).second);
        }
        inline void get_successors(std::vector<Case *> & successor_nodes, const void * a)
        {
                std::map<std::string, Successor *>::iterator itr = _successorlist.begin();
                while(itr != _successorlist.end()) { 
                        Case * s = itr->second->get_successor(a);
                        if(s) {
                                successor_nodes.push_back(s);
                        }
                        itr++;
                }
        }
        void pretty_print(int depth, std::set<std::string> * visited);
private:
        std::map<std::string, Successor *> _successorlist;
};


/**
 * @class Node
 * @brief holds the programmatic information about a node (err handler, succ list)
 * Holds the name, error handler, event factory function, 
 * and flow successor list of a flow node.  
 * Also maintains the info that the node is a source (if it is).
 */
class Node {
public:
        friend class NodeCounterIncrementer;
        Node(const char * name,
                CreateNodeFn createfn,
                bool is_error_handler,
                bool is_source,
                bool is_detached,
                int input_unionnumber,
                int output_unionnumber);
        ~Node();
        void setErrorHandler(Node *fn);
        SuccessorList & successor_list() { return _successor_list; }
        inline const char * getName() { return &(_name[0]); }
        inline bool getIsSource() { return _is_source; }
        inline bool getIsErrorHandler() { return _is_error_handler; }
        inline bool getIsDetached() { return _is_detached; }
        inline CreateNodeFn & getCreateFn() { return _createfn; }
        inline void get_successors(std::vector<Case *> & successor_nodes, 
                        const void * a,
                        int return_code)
        {
                if(return_code != 0) {
                        if(_error_handler_case.targetNode() != NULL) {
                                successor_nodes.push_back(&_error_handler_case);
                        }
			if(_is_source) { // even on errors
                                successor_nodes.push_back(&_this_case);
                        }
                } else {
                        _successor_list.get_successors(successor_nodes,a);
                }
        }
        inline std::vector<GuardReference *> & guards() { return _guard_refs; }
        inline void addGuard(GuardReference * fgr) 
        { 
                fgr->setLexicalIndex(_guard_refs.size());
                _guard_refs.push_back(fgr); 
        }
        void log_snapshot();
        void pretty_print(int depth, char context, std::set<std::string> * visited);
#ifdef PROFILING
        inline TimerStats * real_timer_stats() { return &_real_timer_stats; }
        inline TimerStats * oflux_timer_stats() { return &_oflux_timer_stats; }
#endif
        inline int instances() { return _instances; }
        inline void turn_off_source() { _is_source = false; }
        inline int inputUnionNumber() { return _input_unionnumber; }
        inline int outputUnionNumber() { return _output_unionnumber; }
        void sortGuards();
        bool isGuardsCompletelySorted() { return _is_guards_completely_sorted; }
protected:
        int                           _instances; // count active events
        int                           _executions; // count active events
private:
        std::string                   _name;
        CreateNodeFn                  _createfn;
        bool                          _is_error_handler;
        bool                          _is_source;
        bool                          _is_detached;
        SuccessorList                 _successor_list;
        Case                          _error_handler_case;
        Case                          _this_case;
        std::vector<GuardReference *> _guard_refs;
        int                           _input_unionnumber;
        int                           _output_unionnumber;
        bool                          _is_guards_completely_sorted;
#ifdef PROFILING
        TimerStats                    _real_timer_stats;
        TimerStats                    _oflux_timer_stats;
#endif
};

class NodeCounterIncrementer {
public:
        NodeCounterIncrementer(Node * flow_node)
                : _flow_node(flow_node)
        { 
                _flow_node->_instances++; 
                _flow_node->_executions++; 
        }
        virtual ~NodeCounterIncrementer();
        inline Node * flow_node() { return _flow_node; }
protected:
        Node * _flow_node;
};


class Flow;

class GuardMagicSorter : public MagicSorter {
public:
        GuardMagicSorter(Flow * f) : _flow(f) {}
        virtual MagicNumberable * getMagicNumberable(const char * c);
private:
        Flow * _flow;
};

class Library;
/**
 * @class Flow
 * @brief a flow is a collection of flow nodes (connected internally)
 */
class Flow {
public:
        Flow() 
		: _magic_sorter(this) 
		, _gaveup_libraries(false)
	{}
        Flow(const Flow &);
        ~Flow();

        /**
         * @brief get a vector of sources in the flow
         * @return a vector of sources
         **/
        std::vector<Node *> & sources();

        /**
         * @brief add a flow node
         * @param fn Node to add to the flow
         **/
        void add(Node *fn)
        {
                std::pair<std::string,Node *> pr(fn->getName(),fn);
                _nodes.insert(pr);
                if(fn->getIsSource()) {
                        _sources.push_back(fn);
                }
        }

        /**
         * @brief obtain a flow node by name
         * @param name given a name of a flow node
         * @return the flow node of that name (NULL for not found)
         **/
        Node * get(const std::string & name)
        {
                std::map<std::string, Node *>::iterator itr = _nodes.find(name);
                return (itr == _nodes.end() ? NULL : (*itr).second);
        }

        /**
         * @brief lookup a guard with a particular name
         * @param name of the guard you are looking for
         * @return a pointer to the guard (NULL if not found)
         */
        Guard * getGuard(const std::string & name)
        {
                std::map<std::string, Guard *>::iterator itr = _guards.find(name);
                return (itr == _guards.end() ? NULL : (*itr).second);
        }

        /**
         * @brief add a guard to the guard map (internal) for lookup
         * @param fg the guard to add to this flow
         */
        void addGuard(Guard *fg) 
        {
                _guards[fg->getName()] = fg;
        }
        /**
         * @brief output to the logger some information about the state in
         * this object(flow)
         */
        void log_snapshot();
        void log_snapshot_guard(const char * gname);
        /**
         * @brief log a "pretty printed" flow DAG (graph) showing the flow
         */
        void pretty_print();
        /**
         * @brief take the source property away from the node in this flow
         *   that currently have it (they will no longer spawn themselves)
         */
        void turn_off_sources()
        {
                for(int i = 0; i < (int)_sources.size(); i++) {
                        _sources[i]->turn_off_source();
                }
                _sources.clear();
        }
        /**
         * @brief determine whether there are event instances using this flow
         */
        bool has_instances() 
        {
                bool res = false;
                std::map<std::string, Node *>::iterator mitr = 
                        _nodes.begin();
                while(mitr != _nodes.end() && !res) {
                        res = res || ((*mitr).second->instances() > 0);
                        mitr++;
                }
                return res;
        }
        /**
         * @brief add a library to this flow
         */
        void addLibrary(Library * lib, void * init_plugin_params);
        /**
         * @brief determine if this library is already know to this flow
         */
        bool haveLibrary(const char * name);
	Library * getPrevLibrary(const char * name);
        /**
         * @brief return the names of the plugin libraries in loaded order
         */
        void getLibraryNames(std::vector<std::string> & result);
        /**
         * @brief [internal use] -- total ordering of guards is determined
         */
        void assignMagicNumbers();
        /**
         * @brief add an ordering constraint on the acquisition of two
         *   guards in this flow
         */
        inline void addGuardPrecedence(const char * before, const char * after)
        { _magic_sorter.addInequality(before,after); }
        void drainGuardsOfEvents();
private:
        std::map<std::string, Node *>  _nodes;
        std::vector<Node *>            _sources;
        std::map<std::string, Guard *> _guards;
        GuardMagicSorter               _magic_sorter;
        std::vector<Library *>         _libraries;
        std::vector<Library *>         _prev_libraries;
	mutable bool                   _gaveup_libraries;
};

} // namespace flow

} // namespace oflux

#endif // _OFLUX_FLOW

