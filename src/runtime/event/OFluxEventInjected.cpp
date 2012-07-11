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
