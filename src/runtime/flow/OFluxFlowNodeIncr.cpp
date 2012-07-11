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
	if(_flow_node) {
		_flow_node->_instances++;
	}
}

NodeCounterIncrementer::~NodeCounterIncrementer()
{
    //_flow_node->_instances--;   Now cumulative number
}


} // namespace flow
} // namespace oflux
