#include "event/OFluxEventInjected.h"
#include "flow/OFluxFlowNode.h"
#include "flow/OFluxFlow.h"
#include "OFluxRunTimeAbstract.h"

namespace oflux {

flow::Node *
rt_lookup_flow_node(
	  const std::string & node_name
        , RunTimeAbstract * rt)
{
	return rt ? rt->flow()->get<flow::Node>(node_name) : NULL;
}

namespace injected {

flow::Node the_fn_for_injection(
	  "fn-for-injection"
	, "fun-for-injection"
	, NULL
	, NULL
	, false
	, false
	, false
	, false
	, "0a0"
	, "0b0");

flow::Node * fn_for_injection = &the_fn_for_injection;

} // namespace injected
} // namespace oflux
