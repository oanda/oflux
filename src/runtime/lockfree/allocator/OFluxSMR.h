#ifndef _OFLUX_SMR_H
#define _OFLUX_SMR_H

#include "lockfree/OFluxThreadNumber.h"
#include "lockfree/OFluxMachineSpecific.h"
#include <cstdlib>
#include <cassert>

/**
 * @file OFluxSMR.h
 * @author Mark Pichora
 * SMR (aka hazard pointers)
 *
 * "Safe Memory Reclamation for Dynamic Lock-free Objects Using Atomic Reads
 *  and Writes" by Maged M. Michael, PODC 2002
 */

namespace oflux {
 class AllocatorImplementation;
namespace lockfree {
namespace smr {

#define SMR_NUM_HAZARD_PTR 2

struct HazardPtrForThread {
	void * h[SMR_NUM_HAZARD_PTR];
	char _unused[oflux::lockfree::MachineSpecific::Cache_Line_Size - sizeof(void *)];
};

extern HazardPtrForThread hazard[DEFAULT_MEMPOOL_MAX_THREADS];

enum    { SMR_P = DEFAULT_MEMPOOL_MAX_THREADS // num threads upper bound
	, SMR_K = SMR_NUM_HAZARD_PTR // number of hazard pointers per thread
	, SMR_N = SMR_P * SMR_K // total number of hazard pointers
	, SMR_R = 2 * SMR_N   // batch size
	};

struct DListEntry {
	void * v;
	AllocatorImplementation * alloc;
};


class PerThread {
public:
	void init_PerThread()
	{
		_dcount = 0;
		for(size_t dl_i = 0; dl_i < SMR_R; ++dl_i) {
			_dlist[dl_i].v = NULL;
			_dlist[dl_i].alloc = NULL;
		}
	}
	inline void defer_put(void * v, AllocatorImplementation & ai)
	{
		_dlist[_dcount].v = v;
		_dlist[_dcount].alloc = &ai;
		++_dcount;
		if(_dcount >= SMR_R) {
			scan();
		}
		assert(_dcount < SMR_R);
	}
//protected:
	void scan();
//private:

	DListEntry _dlist[SMR_R];
	size_t _dcount;
};

class DeferFree {
public:
	inline static void defer_put(void * v, AllocatorImplementation & ai)
	{
		_per_thread.defer_put(v,ai);
	}
	inline static void init() // important to call this on thread creation
	{ _per_thread.init_PerThread(); }
private:
	static PerThread __thread _per_thread;
};

/**
 * @brief set the nth hazard pointer to a particular value
 */
template< typename T >
inline void set(T * hp, size_t n)
{
	size_t thr_ind = oflux::lockfree::_tn.index;
	hazard[thr_ind].h[n] = hp;
	oflux::lockfree::mfence_or_equiv_barrier();
}

#define HAZARD_PTR_ASSIGN(H,HFROM,N) \
   H = HFROM; \
   ::oflux::lockfree::smr::set(H,N); \
   if(H != HFROM) continue;

#define HAZARD_PTR_RELEASE(N) \
   ::oflux::lockfree::smr::set((void*)NULL,N);

} // namespace smr
} // namespace lockfree
} // namespace oflux

#endif // _OFLUX_SMR_H
