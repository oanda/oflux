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
#include "flow/OFluxFlowFunctions.h"
#include "flow/OFluxFlowLibrary.h"
#include <cstring>


namespace oflux {
namespace flow {

FunctionMaps::FunctionMaps(ConditionalMap cond_map[],
                ModularCreateMap create_map[],
                ModularCreateDoorMap create_door_map[],
                GuardTransMap guard_map[],
                AtomicMapMap atom_map_map[],
                IOConverterMap ioconverter_map[])
        : _cond_map(cond_map)
        , _create_map(create_map)
        , _create_door_map(create_door_map)
        , _guard_trans_map(guard_map)
        , _atom_map_map(atom_map_map)
        , _ioconverter_map(ioconverter_map)
{}

Library *
FunctionMaps::libraryFactory(const char * dir, const char * name)
{
	return new Library(dir,name);
}

// linear searches are fast enough -- its just for XML converstion...

FlatIOConversionFun 
FunctionMaps::lookup_io_conversion(
	  const char * from_unionhash
	, const char * to_unionhash) const
{
        FlatIOConversionFun res = NULL;
        IOConverterMap * ptr = _ioconverter_map;
        while(ptr->from_unionhash > 0) {
                if(strcmp(ptr->from_unionhash, from_unionhash) == 0 
				&& strcmp(ptr->to_unionhash, to_unionhash) == 0) {
                        res = ptr->conversion_fun;
                        break;
                }
                ptr++;
        }
        return res;
}

template<typename FP>
FP
lookupModularType(ModularFunctionLookup<FP> * mcm, const char * n)
{
        FP res = NULL;
        while(mcm->scope) {
                int sz = strlen(mcm->scope);
                if(strncmp(mcm->scope,n,sz) == 0) {
                        FunctionLookup<FP> * cm = mcm->map;
                        while(cm->name) {
                                if(strcmp(cm->name,n + sz) == 0) {
                                        res = cm->createfn;
                                        break;
                                }
                                cm++;
                        }
                        if(res) {
                                break;
                        }
                }
                mcm++;
        }
        return res;
}

CreateNodeFn 
FunctionMaps::lookup_node_function(const char * n) const
{ return lookupModularType(_create_map,n); }

CreateDoorFn 
FunctionMaps::lookup_door_function(const char * n) const
{ return lookupModularType(_create_door_map,n); }

ConditionFn 
FunctionMaps::lookup_conditional(
	  const char * n
	, int argno
	, const char * unionhash) const
{
        ConditionFn res = NULL;
        ConditionalMap * cm = _cond_map;
        while(cm->name) {
                if(strcmp(cm->name, n) == 0
                        && argno == cm->argno
                        && strcmp(unionhash, cm->unionhash) == 0) {
                        res = cm->condfn;
                        break;
                }
                cm++;
        }
        return res;
}

GuardTransFn 
FunctionMaps::lookup_guard_translator(
	  const char * guardname
	, const char * unionhash
	, const char * hash
	, int wtype
	, bool late) const
{
        GuardTransFn res = NULL;
        GuardTransMap * ptr = _guard_trans_map;
        while(ptr->guardname != NULL) {
                if(strcmp(guardname, ptr->guardname) == 0
                                && strcmp(unionhash, ptr->unionhash) == 0
                                && 0 == strcmp(hash,ptr->hash)
                                && wtype == ptr->wtype
                                && late == ptr->late
                                ) {
                        res = ptr->guardtransfn;
                        break;
                }
                ptr++;
        }
        return res;
}

atomic::AtomicMapAbstract * 
FunctionMaps::lookup_atomic_map(const char * guardname) const
{
        atomic::AtomicMapAbstract * res = NULL;
        AtomicMapMap * ptr = _atom_map_map;
        while(ptr->guardname != NULL) {
                if(strcmp(guardname, ptr->guardname) == 0) {
                        res = *(ptr->amap);
                        break;
                }
                ptr++;
        }
        return res;
}

} // namespace flow
} // namespace oflux
