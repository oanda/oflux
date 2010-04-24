#ifndef OFLUX_THREAD_NUMBER
#define OFLUX_THREAD_NUMBER

#include <cstdlib>
#include <pthread.h>

namespace oflux {
namespace lockfree {

class ThreadNumber {
public:
        static size_t num_threads;
        size_t index;

	static void init(int i);
};

extern __thread ThreadNumber _tn; // thread local access to a thread's index


} // namespace lockfree
} // namespace oflux


#endif // OFLUX_THREAD_NUMBER
