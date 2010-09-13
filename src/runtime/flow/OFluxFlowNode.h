#ifndef _OFLUX_FLOW_NODE
#define _OFLUX_FLOW_NODE

/**
 * @file OFluxFlowNode.h
 * @author Mark Pichora
 * Flow object to hold the details of event node execution.  Each node's
 * C function pointer, list of resources and logic to obtain successors
 * is encapsulated here.
 */

#include "OFlux.h"
#include "OFluxProfiling.h"
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <string>

namespace oflux {
namespace flow {

class Flow;
class GuardReference;
class Case;
class SuccessorList;

/**
 * @class Successor
 * @brief a container for a set of flow cases which are operated in order 
 * The first containing flow case to have all of its conditions met,
 * is used.
 */
class Successor {
public:
	typedef SuccessorList ParentObjType;

        Successor(const char * name);
        ~Successor();
        inline const std::string & getName() { return _name; }
        void add(Case * fc, bool front=false);
	void remove(Case * fc);
	Case * getByTargetName(const char *n);
	Case * getByTargetNode(const Node *n);
        Case * get_successor(const void * a);
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
	typedef Node ParentObjType;

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
	bool has_successor_with_target(const Node * n) const;
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
	typedef Flow ParentObjType;

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
        void successor_list(SuccessorList * sl) { _successor_list = sl; }
        SuccessorList * successor_list() { return _successor_list; }
        inline const char * getName() { return &(_name[0]); }
        inline bool getIsSource() const { return _is_source; }
	bool getIsInitial();
        inline bool getIsErrorHandler() const { return _is_error_handler; }
        inline bool getIsDetached() const { return _is_detached; }
        inline CreateNodeFn & getCreateFn() { return _createfn; }
        void get_successors(std::vector<Case *> & successor_nodes, 
                        const void * a,
                        int return_code);
        inline std::vector<GuardReference *> & guards() { return _guard_refs; }
        void add(GuardReference * fgr);
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
        int                           _is_initial;
        bool                          _is_detached;
        SuccessorList *               _successor_list;
        Case *                        _error_handler_case;
        Case *                        _this_case;
        std::vector<GuardReference *> _guard_refs;
        int                           _input_unionnumber;
        int                           _output_unionnumber;
        bool                          _is_guards_completely_sorted;
#ifdef PROFILING
        TimerStats                    _real_timer_stats;
        TimerStats                    _oflux_timer_stats;
#endif
};

} //namespace flow
} //namespace oflux

#endif

