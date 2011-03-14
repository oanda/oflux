#ifndef _OFLUX_CONDITION
#define _OFLUX_CONDITION

/**
 * @file  OFluxCondition.h
 * @author Mark Pichora
 * template functions for evaluating conditions on specified structures
 */

#include "OFlux.h"

namespace oflux {


/**
 * @brief anonymizer template for a condition function on type T
 * @param t  (anonymous pointer to the object to test)
 * @return true if the condition is true
 **/
template<typename T, bool (*test)(const T *) >
bool eval_condition(const void * t)
{
	//return (test)(std::reinterpret_cast<const T*>(t));
	return (test)(static_cast<const T*>(t));
}

/**
 * @brief anonymizer template for conditional with a select on the field arg
 * @param t (anonymous pointer to the object to test)
 * @return true if the test succeeds
 */

template<typename T, typename Y, bool (*testargn)(const Y),const Y & (T::*getargn)() const >
bool eval_condition_argn(const void * t)
{
	return (*testargn)(((static_cast<const T*> (t))->*getargn)());
}

template<typename T, typename Y, bool (*testargn)(Y const &),const Y & (T::*getargn)() const >
bool eval_condition_argn_amp(const void * t)
{
	return (*testargn)(((static_cast<const T*> (t))->*getargn)());
}

}; // namespace

#endif // _OFLUX_CONDITION
