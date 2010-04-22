
#include "lockfree/atomic/DistributedCounter.h"
#include <pthread.h>

namespace oflux {

size_t ThreadNumber::num_threads = 0;
__thread ThreadNumber _tn = {0};

void init_ThreadNumber(ThreadNumber & tn)
{ 
	tn.index = 0;
	if(!tn.index) {
		tn.index = __sync_fetch_and_add(&ThreadNumber::num_threads,1);
	}
}

} // namespace oflux
