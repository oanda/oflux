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
#include "flow/OFluxFlowNode.h"
#include "flow/OFluxFlowCase.h"
#include "flow/OFluxFlowCommon.h"

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

