#ifndef ATOMIC_INIT_H
#define ATOMIC_INIT_H

namespace oflux {

class AtomicMapAbstract;

class GuardInserter {
public:
	GuardInserter(AtomicMapAbstract *ama)
		: _ama(ama)
		{}
	/**
	 * @brief insert values into a guard
	 */
	bool insert(const void * key, void * value);
private:
	AtomicMapAbstract * _ama;
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
