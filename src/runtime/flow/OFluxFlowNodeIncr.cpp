#include "flow/OFluxFlowNodeIncr.h"
#include "flow/OFluxFlowNode.h"

namespace oflux {
namespace flow {

const char *
get_node_name(const flow::Node *n)
{
	return n->getName();
}

NodeCounterIncrementer::NodeCounterIncrementer(Node * flow_node)
	: _flow_node(flow_node)
{
	_flow_node->_instances++;
}

NodeCounterIncrementer::~NodeCounterIncrementer()
{
    //_flow_node->_instances--;   Now cumulative number
}


} // namespace flow
} // namespace oflux
