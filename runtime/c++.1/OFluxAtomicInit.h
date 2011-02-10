#ifndef ATOMIC_INIT_H
#define ATOMIC_INIT_H

#include <vector>

/**
 * @file OFluxAtomicInit.h
 * @author Mark Pichora
 * @brief These helper classes/macros are used to populate a guard with useful
 *    data, or walk a guard to examine its contents.  The main use of this
 *    functionality is (1) to initiatialize the guard before the runtime
 *    starts and (2) to recover resources when the runtime has finished.
 */

namespace oflux {

class AtomicMapAbstract;

class Atomic;

/**
 * @class GuardInserter
 * @brief Given a pointer to the guard's abstract structure, this object
 *    allows you to insert() (key,value) pairs into the guard (not all
 *    guard types care about the the key argument)
 */
class GuardInserter {
public:
	GuardInserter(AtomicMapAbstract *ama)
		: _ama(ama)
		{}
        ~GuardInserter();
	/**
	 * @brief insert values into a guard
	 * @return false if the value was already populated (non-NULL)
	 */
	bool insert(const void * key, void * value);
	void * swap(const void * key, void * newvalue);
private:
	AtomicMapAbstract *   _ama;
        std::vector<Atomic *> _to_release;
};

class AtomicMapWalker;

/**
 * @class GuardWalker
 * @brief Given a pointer to the guard's abstract structure, this object
 *    allows you to pass through (and modify if you wish), the (key,value)
 *    pairs that are in this guard.  For guard types that do not use
 *    keys,  that argument should be ignored.
 */
class GuardWalker {
public:
        GuardWalker(AtomicMapAbstract *ama);
        ~GuardWalker();
        /**
         * @brief call next() until it returns false. its output parameters
         * give the (key,value) pairs in this guard
         * @return true if there is something "next" and the outputs are valid
         */
        bool next(const void *& key, void **& value);
private:
	AtomicMapWalker *   _walker;
};


#define OFLUX_GUARD_POPULATER_NS(NS,GUARDNAME,POPNAME) \
	oflux::GuardInserter POPNAME(NS::ofluximpl::GUARDNAME##_map_ptr)
#define OFLUX_GUARD_POPULATER(GUARDNAME,POPNAME) \
        OFLUX_GUARD_POPULATER_NS(,GUARDNAME,POPNAME)
#define OFLUX_GUARD_POPULATER_PLUGIN(PLUGIN,GUARDNAME,POPNAME) \
        OFLUX_GUARD_POPULATER_NS(PLUGIN,PLUGIN##_##GUARDNAME,POPNAME)
#define OFLUX_GUARD_CLEAN(GUARDNAME, TYPE) \
        { \
                void ** v; \
                const void * k; \
                OFLUX_GUARD_WALKER(GUARDNAME, Gwalk); \
                while (OFLUX_GUARD_WALKER_NEXT(Gwalk, k, v)) { \
                        if (*v) { \
                                delete static_cast< TYPE *>(*v); \
                        } \
                } \
        }
#define OFLUX_MODULE_INIT_PLUGIN(PLUGIN,MODULENAME,INST, ...) \
        { \
                OFLUX_GUARD_POPULATER_PLUGIN(PLUGIN,INST##_self,instpop); \
                MODULENAME::ModuleConfig * mc = MODULENAME::init( __VA_ARGS__ ); \
                instpop.insert(NULL, mc); \
        }
#define OFLUX_MODULE_INIT_NS(NS,MODULENAME,INST, ...) \
        { \
                OFLUX_GUARD_POPULATER_NS(NS,INST##_self,instpop); \
                MODULENAME::ModuleConfig * mc = MODULENAME::init( __VA_ARGS__ ); \
                instpop.insert(NULL, mc); \
        }
#define OFLUX_MODULE_INIT(MODULENAME,INST, ...) \
        { \
                OFLUX_GUARD_POPULATER(INST##_self,instpop); \
                MODULENAME::ModuleConfig * mc = MODULENAME::init( __VA_ARGS__ ); \
                instpop.insert(NULL, mc); \
        }
#define OFLUX_MODULE_DEINIT(MODULENAME,INST) \
        OFLUX_GUARD_CLEAN(INST##_self, MODULENAME::ModuleConfig)
#define OFLUX_GUARD_WALKER_NS(NS, GUARDNAME,WALKNAME) \
    oflux::GuardWalker WALKNAME(NS::ofluximpl::GUARDNAME##_map_ptr)
#define OFLUX_GUARD_WALKER(GUARDNAME,WALKNAME) \
	OFLUX_GUARD_WALKER_NS(, GUARDNAME,WALKNAME)
#define OFLUX_GUARD_WALKER_NEXT(WALKNAME,K,V) \
        WALKNAME.next(K,V)

} //namespace


#endif // ATOMIC_INIT_H
