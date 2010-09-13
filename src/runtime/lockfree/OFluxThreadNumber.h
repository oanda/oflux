#ifndef OFLUX_THREAD_NUMBER
#define OFLUX_THREAD_NUMBER

/**
 * @file OFluxThreadNumber.h
 * @author Mark Pichora
 *  Portable thread numbering which allows the program to know the total number
 * of threads and the index of the current thread (in thread local storage).
 */

#include <cstdlib>
#include <pthread.h>

namespace oflux {
namespace lockfree {

class ThreadNumber {
public:
        static size_t num_threads;
        size_t index;

	static void init(int i);
	static void init();
};

extern __thread ThreadNumber _tn; // thread local access to a thread's index


} // namespace lockfree
} // namespace oflux


#endif // OFLUX_THREAD_NUMBER
