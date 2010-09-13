#ifndef OFLUX_FLOW_CASE
#define OFLUX_FLOW_CASE

/**
 * @file OFluxFlowCase.h
 * @author Mark Pichora
 * Flow (the dynamic program structure loaded into the runtime) for 
 * conditional logic and concurrent handling.  Determination of successors.
 */

#include "OFlux.h"
#include "OFluxIOConversion.h"
#include <string>
#include <vector>
#include <set>



namespace oflux {
namespace flow {


class Case;

/**
 * @class Condition
 * @brief holder of a conditional (used to determine event flow)
 */
class Condition {
public:
	typedef Case ParentObjType;

        Condition(ConditionFn condfn, bool is_negated);
        inline bool satisfied(const void * a) 
        { return ((*_condfn)(a) ? !_is_negated : _is_negated); }
private:
        ConditionFn _condfn;
        bool        _is_negated;
};

class Flow;

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

class Successor;

/**
 * @class Case
 * @brief given a set of conditions, each case has a node target
 * If ALL conditions are satisfied, then the node target is added to the 
 * event queue.
 */
class Case {
public:
	typedef Successor ParentObjType;

        Case(const char * targetname=""
		, Node *targetnode=NULL
		, IOConverter * converter=NULL);
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
	inline const char * targetNodeName() { return _targetname.c_str(); }
        inline Node * targetNode() { return _targetnode; }
        inline IOConverter * ioConverter() { return _io_converter; }
        inline void setTargetNode(Node * fn) { _targetnode = fn; }
        inline void setIOConverter(IOConverter * fioc) { _io_converter = fioc; }
        void pretty_print(int depth, std::set<std::string> * visited);
        inline bool isDefault() { return _conditions.size() == 0; }
private:
	std::string              _targetname;
        Node *                   _targetnode;
        IOConverter *            _io_converter;
        std::vector<Condition *> _conditions;
};



} // namespace flow
} // namespace oflux


#endif // OFLUX_FLOW_CASE
