#include "OFluxProfiling.h"

namespace oflux {
const char * _cxx_opts= "CXXOPTS="
#ifdef PROFILING
		"-DPROFILING "
#endif
#ifdef SOLARIS_NATIVE_THREADS
		"-DSOLARIS_NATIVE_THREADS "
#endif
#ifdef THREAD_COLLECTION
		"-DTHREAD_COLLECTION "
#endif
	;
}
