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


#define OFLUX_GUARD_POPULATER(GUARDNAME,POPNAME) \
	oflux::GuardInserter POPNAME(ofluximpl::GUARDNAME##_map_ptr)
#define OFLUX_MODULE_INIT(MODULENAME,INST, ...) \
        { \
                OFLUX_GUARD_POPULATER(INST##_self,instpop); \
                MODULENAME::ModuleConfig * mc = MODULENAME::init( __VA_ARGS__ ); \
                instpop.insert(NULL, mc); \
        }

} //namespace


#endif // ATOMIC_INIT_H
