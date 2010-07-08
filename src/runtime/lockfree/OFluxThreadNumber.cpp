#include "OFluxThreadNumber.h"

namespace oflux {
namespace lockfree {

size_t ThreadNumber::num_threads = 0;
__thread ThreadNumber _tn = {0};

void
ThreadNumber::init(int i)
{
	// could have assigned the index implicitly here,
	// but I prefer to guarantee that the ith item is indexed on
	// (i-1) index
	_tn.index = i;
	__sync_fetch_and_add(&ThreadNumber::num_threads,1);
}

void
ThreadNumber::init()
{
	// This version is less used
	_tn.index = __sync_fetch_and_add(&ThreadNumber::num_threads,1);
}


} // namespace lockfree
} // namespace oflux
