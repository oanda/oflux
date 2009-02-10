#ifndef ATOMIC_INIT_H
#define ATOMIC_INIT_H

#include <vector>

namespace oflux {

class AtomicMapAbstract;

class Atomic;

class GuardInserter {
public:
	GuardInserter(AtomicMapAbstract *ama)
		: _ama(ama)
		{}
        ~GuardInserter();
	/**
	 * @brief insert values into a guard
	 */
	bool insert(const void * key, void * value);
private:
	AtomicMapAbstract *   _ama;
        std::vector<Atomic *> _to_release;
};

class AtomicMapWalker;

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
#define OFLUX_MODULE_INIT(MODULENAME,INST, ...) \
        { \
                OFLUX_GUARD_POPULATER(INST##_self,instpop); \
                MODULENAME::ModuleConfig * mc = MODULENAME::init( __VA_ARGS__ ); \
                instpop.insert(NULL, mc); \
        }
#define OFLUX_GUARD_WALKER(GUARDNAME,WALKNAME) \
	oflux::GuardWalker WALKNAME(ofluximpl::GUARDNAME##_map_ptr)
#define OFLUX_GUARD_WALKER_NEXT(WALKNAME,K,V) \
        WALKNAME.next(K,V)

} //namespace


#endif // ATOMIC_INIT_H
