#include "lockfree/allocator/OFluxSMR.h"
#include "OFluxAllocator.h"
#include <cstdlib>


namespace oflux {
namespace lockfree {
namespace smr {

HazardPtrForThread hazard[DEFAULT_MEMPOOL_MAX_THREADS];

static int plist_compare(const void * l, const void * r)
{ return *(void**)l == *(void**)r ? 0 : ( *(void**)l < *(void**)r ? -1 : 1 ); }

inline bool 
binary_search(DListEntry & ent, const size_t plist_size, void * plist[])
{
	void * x = ent.v;
	int low = 0;
	int high = (int)plist_size-1;
	bool res = false;
	while(low <= high) {
		int mid = (low+high) >> 1;
		void * v = plist[mid];
		if(v == x) { 
			res = true;
			break;
		} else if(v < x) {
			low = mid+1;
		} else {
			high = mid-1;
		}
	}
	return res;
}

void
PerThread::scan()
{
// stage 1
	void * h;
	void * plist[SMR_N];
	size_t plist_ind = 0;
	assert(ThreadNumber::num_threads < SMR_P);
	for( size_t thr_i = 0; thr_i < ThreadNumber::num_threads; ++thr_i) {
		for( size_t hp_k = 0; hp_k < SMR_K; ++hp_k) {
			if((h = hazard[thr_i].h[hp_k])) {
				plist[plist_ind++] = h;
			}
		}
	}
// stage 2
	qsort(&plist[0], plist_ind , sizeof(plist[0]), plist_compare);
// stage 3 & 4 (in place)
	int new_dcount = 0;
	for(size_t dl_i = 0; dl_i < _dcount; ++dl_i) {
		if(plist_ind>0 && binary_search(_dlist[dl_i],plist_ind, plist)) {
			_dlist[new_dcount].v = _dlist[dl_i].v;
			_dlist[new_dcount].alloc = _dlist[dl_i].alloc;
			++new_dcount;
		} else {
			// reclaim
			_dlist[dl_i].alloc->put(_dlist[dl_i].v);
			_dlist[dl_i].v = NULL;
			_dlist[dl_i].alloc = NULL;
		}
	}
	_dcount = new_dcount;
}


PerThread __thread DeferFree::_per_thread;



} // namespace smr
} // namespace lockfree
} // namespace oflux
