#include "OFluxFlowNode.h"
#include "OFluxFlowCase.h"
#include "OFluxFlowCommon.h"

namespace oflux {
namespace flow {

Condition::Condition(ConditionFn condfn, bool is_negated)
        : _condfn(condfn)
        , _is_negated(is_negated)
        {}


Case::Case(const char * targetname, Node *targetnode, IOConverter *converter)
        : _targetname(targetname)
	, _targetnode(targetnode)
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




} // namespace flow
} // namespace oflux

