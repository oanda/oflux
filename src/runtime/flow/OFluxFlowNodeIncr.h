#ifndef OFLUX_FLOW_NODE_INCREMENTER
#define OFLUX_FLOW_NODE_INCREMENTER
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
