#ifndef OFLUX_FLOW_FUNCTIONS
#define OFLUX_FLOW_FUNCTIONS

/**
 * @file OFluxFlowFunctions.h
 * @author Mark Pichora
 * Things that hold the compiled-in C-style lookup maps used to load the 
 * program and bind the flow to C function pointers etc when the XML is loaded.
 */

#include "OFlux.h"

namespace oflux {
namespace flow {

/**
 * @class FunctionMaps
 * @brief holds the static maps generated by the flux compiler
 * The static map lookup algorithms are linear (sub-optimal), but this
 * does not matter much.
 */
class FunctionMaps { // data that is compiled in
public:
        FunctionMaps(ConditionalMap cond_map[],
                        ModularCreateMap create_map[],
			ModularCreateDoorMap create_door_map[],
                        GuardTransMap guard_map[],
                        AtomicMapMap atom_map[],
                        IOConverterMap ioconverter_map[]);

        /**
         * @brief lookup a create factory function from a table given the event name
         * @param n name of the node  
         * @return a function pointer usable to create a new event (smart pointered)
         **/
        CreateNodeFn lookup_node_function(const char * n) const;
        CreateDoorFn lookup_door_function(const char * d) const;

        /**
         * @brief lookup a conditional function usable on a particular input
         * @param n  name of the conditional
         * @param argno  the number of the field in the object
         * @param unionnumber  OFluxUnionX number
         * @return 
         **/
        ConditionFn lookup_conditional(const char * n, int argno, int unionnumber) const;
        /**
         * @brief lookup a guard translator function
         * @remark these functions can translate an input structure to a guard key structure
         * @param guardname name of the guard
         * @param union_number union number (indicates the input structure)
         * @param hash is the hash result of on the expression (compiler's)
         * @param wtype is the enumerated type for the guard(Read/Exclusive/...)
         * @return the compiled function (pointer to it)
         */
        GuardTransFn lookup_guard_translator(
		  const char * guardname
                , int union_number
                , const char * hash
                , int wtype
		, bool late) const;

        /**
         * @brief lookup the atomic map object for the given guard 
         * @param guardname the guard name
         * @return the atomic map object
         */
        atomic::AtomicMapAbstract * 
	lookup_atomic_map(const char * guardname) const;

        /**
         * @brief lookup the conversion from one type union to another
         * @return a generic function that can create an object that does the job
         */
        FlatIOConversionFun lookup_io_conversion(int from_unionnumber, int to_unionnumber) const;
private:
        ConditionalMap *   _cond_map;
        ModularCreateMap * _create_map;
        ModularCreateDoorMap * _create_door_map;
        GuardTransMap *    _guard_trans_map;
        AtomicMapMap *     _atom_map_map;
        IOConverterMap *   _ioconverter_map;
};



} // namespace flow
} // namespace oflux




#endif // OFLUX_FLOW_FUNCTIONS
