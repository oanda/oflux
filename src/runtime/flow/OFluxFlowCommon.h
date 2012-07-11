#ifndef OFLUX_FLOW_COMMON
#define OFLUX_FLOW_COMMON
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
 * @file OFluxFlowCommon.h
 * @author Mark Pichora
 * Some useful utilities for deletion of container heap allocated objects
 */

#include <vector>
#include <map>
#include <deque>

namespace oflux {
namespace flow {

template<class O>
inline void 
delete_vector_contents(std::vector<O *> & vo)
{
        for(int i = 0; i < (int) vo.size(); i++) {
                delete vo[i];
        }
}

template<class O>
inline void 
delete_vector_contents_reversed(std::vector<O *> & vo)
{
        for(int i = ((int)vo.size())-1; i >= 0 ; --i) {
                delete vo[i];
        }
}

template<class O>
inline void 
delete_deque_contents(std::deque<O *> & mo)
{
        typename std::deque<O *>::iterator itr;
        itr = mo.begin();
        while(itr != mo.end()) {
                delete (*itr);
                itr++;
        }
}

template<typename K, typename O>
inline void 
delete_map_contents(std::map<K,O *> & mo)
{
        typename std::map<K,O *>::iterator mitr;
        mitr = mo.begin();
        while(mitr != mo.end()) {
                delete (*mitr).second;
                mitr++;
        }
}

} // namespace flow
} // namespace oflux


#endif // OFLUX_FLOW_COMMON
