#ifndef OFLUX_FLOW_NODE_INCREMENTER
#define OFLUX_FLOW_NODE_INCREMENTER

/**
 * @file OFluxFlowNodeIncr.h
 * @author Mark Pichora
 * RAII class which does counter inrementing to indicate the number
 * of instances of flow nodes created (so events).
 */

namespace oflux {
namespace flow {

class Node;

extern const char * get_node_name(const flow::Node *n);

class NodeCounterIncrementer {
public:
        NodeCounterIncrementer(Node * flow_node);
        virtual ~NodeCounterIncrementer();
        inline Node * flow_node() { return _flow_node; }
protected:
        Node * _flow_node;
};


} // namespace flow
} // namespace oflux




#endif // OFLUX_FLOW_NODE_INCREMENTER
