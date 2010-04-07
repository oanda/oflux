#ifndef OFLUX_FLOW_NODE_INCREMENTER
#define OFLUX_FLOW_NODE_INCREMENTER

namespace oflux {
namespace flow {

class Node;

extern const char * get_node_name(flow::Node *n);

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
